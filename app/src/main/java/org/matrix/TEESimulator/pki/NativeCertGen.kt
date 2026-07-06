package org.matrix.TEESimulator.pki

import java.io.ByteArrayInputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.security.KeyFactory
import java.security.KeyPair
import java.security.cert.Certificate
import java.security.cert.CertificateFactory
import java.security.spec.PKCS8EncodedKeySpec
import org.matrix.TEESimulator.logging.SystemLogger

data class CertGenConfig(
    val algorithm: Int,
    val keySize: Int,
    val ecCurve: Int,
    val rsaPublicExponent: Long,
    val attestationChallenge: ByteArray?,
    val purposes: IntArray,
    val digests: IntArray,
    val certSerial: ByteArray?,
    val certSubject: ByteArray?,
    val certNotBefore: Long,
    val certNotAfter: Long,
    val keyboxPrivateKey: ByteArray,
    val keyboxCertChain: ByteArray,
    val securityLevel: Int,
    val attestVersion: Int,
    val keymasterVersion: Int,
    val osVersion: Int,
    val osPatchLevel: Int,
    val vendorPatchLevel: Int,
    val bootPatchLevel: Int,
    val bootKey: ByteArray,
    val bootHash: ByteArray,
    val creationDatetime: Long,
    val attestationApplicationId: ByteArray,
    val moduleHash: ByteArray?,
    val idBrand: ByteArray?,
    val idDevice: ByteArray?,
    val idProduct: ByteArray?,
    val idSerial: ByteArray?,
    val idImei: ByteArray?,
    val idMeid: ByteArray?,
    val idManufacturer: ByteArray?,
    val idModel: ByteArray?,
    val idSecondImei: ByteArray?,
    val activeDatetime: Long = -1L,
    val originationExpireDatetime: Long = -1L,
    val usageExpireDatetime: Long = -1L,
    val usageCountLimit: Int = -1,
    val callerNonce: Boolean = false,
    val unlockedDeviceRequired: Boolean = false,
    val noAuthRequired: Boolean = true,
)

object NativeCertGen {

    private val OBF_KEY =
        byteArrayOf(75, 57, 120, 35, 109, 80, 50, 36, 118, 76, 55, 110, 81, 52, 119, 90)

    private fun xorDec(b: ByteArray): String {
        val out = ByteArray(b.size)
        for (i in b.indices) out[i] = (b[i].toInt() xor OBF_KEY[i % OBF_KEY.size].toInt()).toByte()
        return String(out, Charsets.US_ASCII)
    }

    private val LOG_DIR =
        xorDec(byteArrayOf(100, 93, 25, 87, 12, 127, 83, 64, 20, 99, 81, 15, 37, 81, 31, 109)) + "/logs"

    @Volatile
    var isAvailable: Boolean = false
        private set

    fun initialize(libraryPath: String) {
        try {
            System.load(libraryPath)
            initLogging(false, LOG_DIR)
            isAvailable = true
            SystemLogger.info("NativeCertGen: loaded libcertgen.so successfully")
        } catch (e: UnsatisfiedLinkError) {
            SystemLogger.error("NativeCertGen: failed to load libcertgen.so, falling back to BouncyCastle", e)
        }
    }

    external fun generateAttestedKeyPair(config: CertGenConfig): ByteArray?

    private external fun initLogging(verbose: Boolean, logDir: String): Boolean

    private external fun dumpLogs(): String?

    fun dump(): String? = if (isAvailable) dumpLogs() else null

    fun parseNativeResult(bytes: ByteArray): Pair<KeyPair, List<Certificate>> {
        val buf = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN)

        val pkLen = buf.getInt()
        if (pkLen < 0 || pkLen > buf.remaining()) {
            throw IllegalStateException("Invalid private key length: $pkLen")
        }
        val pkBytes = ByteArray(pkLen)
        buf.get(pkBytes)

        val numCerts = buf.getInt()
        if (numCerts < 0 || numCerts > buf.remaining()) {
            throw IllegalStateException("Invalid cert count: $numCerts")
        }
        val certs = mutableListOf<Certificate>()
        val certFactory = CertificateFactory.getInstance("X.509")
        repeat(numCerts) {
            val certLen = buf.getInt()
            if (certLen < 0 || certLen > buf.remaining()) {
                throw IllegalStateException("Invalid cert length: $certLen")
            }
            val certBytes = ByteArray(certLen)
            buf.get(certBytes)
            certs.add(certFactory.generateCertificate(ByteArrayInputStream(certBytes)))
        }

        if (certs.isEmpty()) {
            throw IllegalStateException("No certificates in native result")
        }

        val algorithmName = when (certs[0].publicKey.algorithm) {
            "EC", "ECDSA" -> "EC"
            "RSA" -> "RSA"
            else -> certs[0].publicKey.algorithm
        }
        val keyFactory = KeyFactory.getInstance(algorithmName)
        val privateKey = keyFactory.generatePrivate(PKCS8EncodedKeySpec(pkBytes))
        val publicKey = certs[0].publicKey
        return Pair(KeyPair(publicKey, privateKey), certs)
    }
}
