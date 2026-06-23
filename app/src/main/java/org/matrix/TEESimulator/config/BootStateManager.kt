package org.matrix.TEESimulator.config

import android.os.SystemProperties
import org.matrix.TEESimulator.logging.SystemLogger
import org.matrix.TEESimulator.util.AndroidDeviceUtils

object BootStateManager {
    private val targets =
        linkedMapOf(
            "ro.boot.verifiedbootstate" to "green",
            "ro.boot.flash.locked" to "1",
            "ro.boot.veritymode" to "enforcing",
            "ro.boot.vbmeta.device_state" to "locked",
        )

    private val fillIfAbsent =
        linkedMapOf(
            "ro.boot.vbmeta.invalidate_on_error" to "yes",
            "ro.boot.vbmeta.avb_version" to "1.2",
            "ro.boot.vbmeta.hash_alg" to "sha256",
            "ro.boot.vbmeta.size" to "11904",
        )

    fun apply() {
        for ((name, target) in targets) {
            val current = SystemProperties.get(name, "")
            if (current.isEmpty()) {
                SystemLogger.debug("BootStateManager: $name absent on this device, skip")
                continue
            }
            if (current == target) {
                SystemLogger.debug("BootStateManager: $name already $target, skip")
                continue
            }
            SystemLogger.info("BootStateManager: setting $name=$target (was: '$current')")
            AndroidDeviceUtils.setProperty(name, target)
        }
        for ((name, value) in fillIfAbsent) {
            val current = SystemProperties.get(name, "")
            if (current.isNotEmpty()) {
                SystemLogger.debug("BootStateManager: $name already '$current', skip")
                continue
            }
            SystemLogger.info("BootStateManager: filling absent $name=$value")
            AndroidDeviceUtils.setProperty(name, value)
        }
    }
}
