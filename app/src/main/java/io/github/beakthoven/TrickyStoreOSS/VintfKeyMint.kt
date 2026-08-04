/*
 * VINTF KeyMint version reader — ported from TEESimulator-RS 307.
 * Reads the device's real KeyMint/Keymaster HAL version from VINTF manifests,
 * so attestation version adapts to the actual device instead of an SDK guess.
 * This widens device support (devices whose HAL version differs from the SDK map).
 */

package io.github.beakthoven.TrickyStoreOSS

import android.util.Log
import io.github.beakthoven.TrickyStoreOSS.logging.TAG
import java.io.File
import javax.xml.parsers.DocumentBuilderFactory
import org.w3c.dom.Element

object VintfKeyMint {

    data class Version(
        val attestationVersion: Int,
        val keymasterVersion: Int,
        val sourcePath: String,
    )

    /** Real KeyMint version from device VINTF manifests, or null when none is readable. */
    val version: Version? by lazy {
        readVintfKeyMintVersion().also { v ->
            if (v != null) {
                Log.i(TAG, "Using KeyMint version from VINTF: attestation=${v.attestationVersion}, keymaster=${v.keymasterVersion}, source=${v.sourcePath}")
            } else {
                Log.d(TAG, "No usable KeyMint VINTF declaration found; using fallback")
            }
        }
    }

    private fun readVintfKeyMintVersion(): Version? {
        val files = linkedMapOf<String, File>()

        VINTF_MANIFEST_DIRS.forEach { path ->
            val dir = File(path)
            if (!dir.exists() || !dir.isDirectory) return@forEach
            val listed = runCatching {
                dir.listFiles { file -> file.isFile && file.name.endsWith(".xml", ignoreCase = true) }
            }.getOrNull()
            listed?.forEach { file -> files[file.absolutePath] = file }
        }

        VINTF_MANIFEST_FILES.forEach { path ->
            val file = File(path)
            if (file.exists() && file.isFile) files[file.absolutePath] = file
        }

        return files.values
            .flatMap { file ->
                runCatching { parseKeyMintVersions(file) }.getOrElse { emptyList() }
            }
            .maxByOrNull { it.attestationVersion }
    }

    private fun parseKeyMintVersions(file: File): List<Version> {
        val document = DocumentBuilderFactory.newInstance().newDocumentBuilder().parse(file)
        val root = document.documentElement ?: return emptyList()

        return directChildElements(root, "hal").flatMap { hal ->
            val halName = directChildTexts(hal, "name").firstOrNull().orEmpty()
            val versions = directChildTexts(hal, "version")
            val fqnames = directChildTexts(hal, "fqname")
            val interfaces = directChildElements(hal, "interface").associate { ie ->
                val name = directChildTexts(ie, "name").firstOrNull().orEmpty()
                val instances = directChildTexts(ie, "instance").toSet()
                name to instances
            }

            when (halName) {
                KEYMINT_HAL_NAME ->
                    if (hasDefaultInstance(fqnames, interfaces, KEYMINT_INTERFACE_NAME)) {
                        versions.mapNotNull { v ->
                            v.toIntOrNull()?.takeIf { it > 0 }?.let { aidl ->
                                val av = aidl * 100
                                Version(av, av, file.absolutePath)
                            }
                        }
                    } else emptyList()
                KEYMASTER_HAL_NAME ->
                    if (hasDefaultInstance(fqnames, interfaces, KEYMASTER_INTERFACE_NAME)) {
                        (versions.flatMap(::expandHidlVersions) + fqnames.mapNotNull(::versionFromFqname))
                            .distinct()
                            .mapNotNull { v ->
                                expectedLegacyVersions(v)?.let { exp ->
                                    Version(exp.second, exp.first, file.absolutePath)
                                }
                            }
                    } else emptyList()
                else -> emptyList()
            }
        }
    }

    private fun directChildElements(parent: Element, tagName: String): List<Element> = buildList {
        val children = parent.childNodes
        for (index in 0 until children.length) {
            val child = children.item(index)
            if (child is Element && child.tagName == tagName) add(child)
        }
    }

    private fun directChildTexts(parent: Element, tagName: String): List<String> =
        directChildElements(parent, tagName).map { it.textContent.trim() }.filter { it.isNotEmpty() }

    private fun hasDefaultInstance(
        fqnames: List<String>,
        interfaces: Map<String, Set<String>>,
        interfaceName: String,
    ): Boolean =
        fqnames.any { fqname ->
            fqname.substringAfter("::", fqname).substringBefore("/") == interfaceName &&
                fqname.substringAfter("/", "") == DEFAULT_INSTANCE
        } || interfaces[interfaceName]?.contains(DEFAULT_INSTANCE) == true

    private fun versionFromFqname(fqname: String): String? =
        FQNAME_VERSION_REGEX.find(fqname)?.groupValues?.getOrNull(1)

    private fun expectedLegacyVersions(version: String): Pair<Int, Int>? =
        when (version) {
            "3.0" -> 3 to 2
            "4.0" -> 4 to 3
            "4.1" -> 41 to 4
            else -> null
        }

    private fun expandHidlVersions(version: String): List<String> {
        val range = HIDL_VERSION_RANGE_REGEX.matchEntire(version) ?: return listOf(version)
        val major = range.groupValues[1]
        val firstMinor = range.groupValues[2].toInt()
        val lastMinor = range.groupValues[3].toInt()
        return (firstMinor..lastMinor).map { minor -> "$major.$minor" }
    }

    private val VINTF_MANIFEST_DIRS = listOf(
        "/system/etc/vintf/manifest",
        "/system_ext/etc/vintf/manifest",
        "/product/etc/vintf/manifest",
        "/vendor/etc/vintf/manifest",
        "/odm/etc/vintf/manifest",
    )
    private val VINTF_MANIFEST_FILES = listOf(
        "/system/etc/vintf/manifest.xml",
        "/system_ext/etc/vintf/manifest.xml",
        "/product/etc/vintf/manifest.xml",
        "/vendor/etc/vintf/manifest.xml",
        "/odm/etc/vintf/manifest.xml",
    )
    private const val KEYMINT_HAL_NAME = "android.hardware.security.keymint"
    private const val KEYMASTER_HAL_NAME = "android.hardware.keymaster"
    private const val KEYMINT_INTERFACE_NAME = "IKeyMintDevice"
    private const val KEYMASTER_INTERFACE_NAME = "IKeymasterDevice"
    private const val DEFAULT_INSTANCE = "default"
    private val FQNAME_VERSION_REGEX = Regex("^@([0-9]+(?:\\.[0-9]+)?)::")
    private val HIDL_VERSION_RANGE_REGEX = Regex("^([0-9]+)\\.([0-9]+)-([0-9]+)$")
}
