/*
 * Soter service forge — ported from TEESimulator-RS 307, adapted to V3.
 * Forges healthy com.tencent.soter.soterserver.ISoterService replies so the
 * SOTER capability probe (DuckDetector) reads available=true / damaged=false.
 */

package io.github.beakthoven.TrickyStoreOSS.interceptors

import android.os.IBinder
import android.os.Parcel
import android.util.Base64
import android.util.Log
import io.github.beakthoven.TrickyStoreOSS.logging.TAG
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.security.KeyPairGenerator

object SoterInterceptor : BinderInterceptor() {

    const val DESCRIPTOR = "com.tencent.soter.soterserver.ISoterService"

    // AIDL transaction codes (1..13 in declaration order)
    private const val TX_GENERATE_APP_SECURE_KEY = 1
    private const val TX_GET_APP_SECURE_KEY = 2
    private const val TX_HAS_ASK_ALREADY = 3
    private const val TX_GENERATE_AUTH_KEY = 4
    private const val TX_REMOVE_AUTH_KEY = 5
    private const val TX_GET_AUTH_KEY = 6
    private const val TX_REMOVE_ALL_AUTH_KEY = 7
    private const val TX_HAS_AUTH_KEY = 8
    private const val TX_INIT_SIGH = 9
    private const val TX_FINISH_SIGN = 10
    private const val TX_GET_DEVICE_ID = 11
    private const val TX_GET_VERSION = 12
    private const val TX_GET_EXTRA_PARAM = 13

    private const val SOTER_OK = 0
    private const val SIGNATURE_LEN = 256
    private const val CPU_ID = "0000000000000000"

    private val forgedCodes = intArrayOf(
        TX_GENERATE_APP_SECURE_KEY, TX_GET_APP_SECURE_KEY, TX_HAS_ASK_ALREADY,
        TX_GENERATE_AUTH_KEY, TX_REMOVE_AUTH_KEY, TX_GET_AUTH_KEY,
        TX_REMOVE_ALL_AUTH_KEY, TX_HAS_AUTH_KEY, TX_INIT_SIGH,
        TX_FINISH_SIGN, TX_GET_DEVICE_ID, TX_GET_VERSION, TX_GET_EXTRA_PARAM,
    )

    override val interceptedCodes: IntArray
        get() = forgedCodes

    private val signatureBlob = ByteArray(SIGNATURE_LEN)
    private val deviceIdBlob = "TEESIM-SOTER-0001".toByteArray(Charsets.UTF_8)
    private val exportBlob: ByteArray by lazy { buildExportBlob() }

    private fun buildExportBlob(): ByteArray {
        val pubKey = runCatching {
            val generator = KeyPairGenerator.getInstance("RSA").apply { initialize(2048) }
            Base64.encodeToString(generator.generateKeyPair().public.encoded, Base64.NO_WRAP)
        }.getOrDefault("")
        val json = """{"pub_key":"$pubKey","counter":0,"cpu_id":"$CPU_ID","uid":0}"""
            .toByteArray(Charsets.UTF_8)
        val lengthPrefix = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
            .putInt(json.size).array()
        return lengthPrefix + json + signatureBlob
    }

    override fun onPreTransact(
        target: IBinder,
        code: Int,
        flags: Int,
        callingUid: Int,
        callingPid: Int,
        data: Parcel,
    ): Result {
        return when (code) {
            TX_GENERATE_APP_SECURE_KEY,
            TX_GENERATE_AUTH_KEY,
            TX_REMOVE_AUTH_KEY,
            TX_REMOVE_ALL_AUTH_KEY -> forgedReply { writeInt(SOTER_OK) }
            TX_GET_VERSION -> forgedReply { writeInt(1) }
            TX_HAS_ASK_ALREADY,
            TX_HAS_AUTH_KEY -> forgedReply { writeInt(1) }

            TX_GET_APP_SECURE_KEY,
            TX_GET_AUTH_KEY -> forgedReply {
                writeInt(1)
                writeInt(SOTER_OK)
                writeByteArray(exportBlob)
                writeInt(exportBlob.size)
            }
            TX_INIT_SIGH -> forgedReply {
                writeInt(1)
                writeLong(1L)
                writeInt(SOTER_OK)
            }
            TX_FINISH_SIGN -> forgedReply {
                writeInt(1)
                writeInt(SOTER_OK)
                writeByteArray(signatureBlob)
                writeInt(signatureBlob.size)
            }
            TX_GET_DEVICE_ID -> forgedReply {
                writeInt(1)
                writeInt(SOTER_OK)
                writeByteArray(deviceIdBlob)
                writeInt(deviceIdBlob.size)
            }
            TX_GET_EXTRA_PARAM -> forgedReply {
                writeInt(1)
                writeValue("optical")
            }
            else -> Continue
        }
    }

    private inline fun forgedReply(body: Parcel.() -> Unit): OverrideReply {
        val reply = Parcel.obtain()
        reply.writeNoException()
        reply.body()
        return OverrideReply(reply = reply)
    }
}
