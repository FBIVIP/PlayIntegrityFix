package org.matrix.TEESimulator.interception.keystore

import android.hardware.security.keymint.KeyParameter
import android.hardware.security.keymint.KeyParameterValue
import android.hardware.security.keymint.Tag
import android.os.Parcel
import android.os.Parcelable
import android.security.KeyStore
import android.security.keystore.KeystoreResponse
import android.system.keystore2.Authorization
import org.matrix.TEESimulator.interception.core.BinderInterceptor
import org.matrix.TEESimulator.logging.SystemLogger
import org.matrix.TEESimulator.util.AndroidDeviceUtils

data class KeyIdentifier(val uid: Int, val alias: String)

/** A collection of utility functions to support binder interception. */
object InterceptorUtils {

    private const val EX_SERVICE_SPECIFIC = -8

    private fun synthesizeSseMessage(errorCode: Int): String =
        when (errorCode) {
            2 -> "Error::Rc(SYSTEM_ERROR)"
            4 -> "Error::Rc(PERMISSION_DENIED)"
            6 -> "Error::Rc(VALUE_CORRUPTED)"
            7 -> "Error::Rc(KEY_NOT_FOUND)"
            10 -> "Error::Rc(BACKEND_BUSY)"
            -3 -> "Error::Km(UNSUPPORTED_KEY_SIZE)"
            -6 -> "Error::Km(INCOMPATIBLE_PURPOSE)"
            -7 -> "Error::Km(INCOMPATIBLE_ALGORITHM)"
            -29 -> "Error::Km(TOO_MANY_OPERATIONS)"
            -49 -> "Error::Km(UNSUPPORTED_TAG)"
            -75 -> "Error::Km(INVALID_INPUT_LENGTH)"
            -76 -> "Error::Km(INVALID_TAG)"
            else -> if (errorCode > 0) "Error::Rc($errorCode)" else "Error::Km($errorCode)"
        }

    fun createErrorReply(errorCode: Int): BinderInterceptor.TransactionResult.OverrideReply {
        val parcel = Parcel.obtain().apply {
            writeInt(EX_SERVICE_SPECIFIC)
            writeString(synthesizeSseMessage(errorCode))
            writeInt(0) // empty remote stack trace header (AOSP Status.cpp:196)
            writeInt(errorCode)
        }
        return BinderInterceptor.TransactionResult.OverrideReply(parcel)
    }

    /**
     * Uses reflection to get the integer transaction code for a given method name from a Stub
     * class. This is necessary for older Android versions where codes are not public constants.
     */
    fun getTransactCode(clazz: Class<*>, method: String): Int {
        return try {
            clazz.getDeclaredField("TRANSACTION_$method").apply { isAccessible = true }.getInt(null)
        } catch (e: Exception) {
            SystemLogger.error(
                "Failed to get transaction code for method '$method' in class '${clazz.simpleName}'.",
                e,
            )
            -1 // Return an invalid code
        }
    }

    /** Creates an `KeystoreResponse` parcel that indicates success with no data. */
    fun createSuccessKeystoreResponse(): KeystoreResponse {
        val parcel = Parcel.obtain()
        try {
            parcel.writeInt(KeyStore.NO_ERROR)
            parcel.writeString("")
            parcel.setDataPosition(0)
            return KeystoreResponse.CREATOR.createFromParcel(parcel)
        } finally {
            parcel.recycle()
        }
    }

    /** Creates an `OverrideReply` parcel that indicates success with no data. */
    fun createSuccessReply(
        writeResultCode: Boolean = true
    ): BinderInterceptor.TransactionResult.OverrideReply {
        val parcel =
            Parcel.obtain().apply {
                writeNoException()
                if (writeResultCode) {
                    writeInt(KeyStore.NO_ERROR)
                }
            }
        return BinderInterceptor.TransactionResult.OverrideReply(parcel)
    }

    /** Creates an `OverrideReply` parcel containing a raw byte array. */
    fun createByteArrayReply(data: ByteArray): BinderInterceptor.TransactionResult.OverrideReply {
        val parcel =
            Parcel.obtain().apply {
                writeNoException()
                writeByteArray(data)
            }
        return BinderInterceptor.TransactionResult.OverrideReply(parcel)
    }

    /** Creates an `OverrideReply` parcel containing a typed array. */
    fun <T : Parcelable> createTypedArrayReply(
        array: Array<T>,
        flags: Int = 0,
    ): BinderInterceptor.TransactionResult.OverrideReply {
        val parcel =
            Parcel.obtain().apply {
                writeNoException()
                writeTypedArray(array, flags)
            }
        return BinderInterceptor.TransactionResult.OverrideReply(parcel)
    }

    /** Creates an `OverrideReply` parcel containing a Parcelable object. */
    fun <T : Parcelable?> createTypedObjectReply(
        obj: T,
        flags: Int = 0,
        diagnosticTag: String? = null,
    ): BinderInterceptor.TransactionResult.OverrideReply {
        val parcel =
            Parcel.obtain().apply {
                writeNoException()
                writeTypedObject(obj, flags)
            }
        if (diagnosticTag != null && SystemLogger.isDebugBuild) {
            val savedPos = parcel.dataPosition()
            val wire = parcel.marshall()
            parcel.setDataPosition(savedPos)
            val path = "/data/local/tmp/teesim-$diagnosticTag.bin"
            runCatching { java.io.File(path).writeBytes(wire) }
            SystemLogger.debug("[$diagnosticTag] reply len=${wire.size} path=$path")
        }
        return BinderInterceptor.TransactionResult.OverrideReply(parcel)
    }

    /**
     * Extracts the base alias from a potentially prefixed alias string. For example, it converts
     * "USRCERT_my_key" to "my_key".
     */
    fun extractAlias(prefixedAlias: String): String {
        val underscoreIndex = prefixedAlias.indexOf('_')
        return if (underscoreIndex != -1) {
            // Return the part of the string after the first underscore.
            prefixedAlias.substring(underscoreIndex + 1)
        } else {
            // If there's no underscore, return the original string.
            prefixedAlias
        }
    }

    /** Checks if a reply parcel contains an exception without consuming it. */
    fun hasException(reply: Parcel): Boolean {
        val exception = runCatching { reply.readException() }.exceptionOrNull()
        if (exception != null) reply.setDataPosition(0)
        return exception != null
    }

    fun createServiceSpecificErrorReply(
        errorCode: Int
    ): BinderInterceptor.TransactionResult.OverrideReply = createErrorReply(errorCode)

    fun normalizeServiceSpecificReply(reply: Parcel): Parcel? {
        reply.setDataPosition(0)
        if (reply.readInt() != EX_SERVICE_SPECIFIC) {
            reply.setDataPosition(0)
            return null
        }
        // Advance position past message and stack header to reach errorCode.
        reply.readString()
        reply.readInt()
        val errorCode = reply.readInt()
        reply.setDataPosition(0)
        return Parcel.obtain().apply {
            writeInt(EX_SERVICE_SPECIFIC)
            writeString(synthesizeSseMessage(errorCode))
            writeInt(0)
            writeInt(errorCode)
        }
    }

    fun patchAuthorizations(
        authorizations: Array<Authorization>?,
        callingUid: Int,
    ): Array<Authorization>? {
        if (authorizations == null) return null

        val osPatch = AndroidDeviceUtils.getPatchLevel(callingUid)
        val vendorPatch = AndroidDeviceUtils.getVendorPatchLevelLong(callingUid)
        val bootPatch = AndroidDeviceUtils.getBootPatchLevelLong(callingUid)

        return authorizations
            .map { auth ->
                val replacement =
                    when (auth.keyParameter.tag) {
                        Tag.OS_PATCHLEVEL ->
                            if (osPatch != AndroidDeviceUtils.DO_NOT_REPORT) osPatch else null
                        Tag.VENDOR_PATCHLEVEL ->
                            if (vendorPatch != AndroidDeviceUtils.DO_NOT_REPORT) vendorPatch
                            else null
                        Tag.BOOT_PATCHLEVEL ->
                            if (bootPatch != AndroidDeviceUtils.DO_NOT_REPORT) bootPatch else null
                        else -> null
                    }
                if (replacement != null) {
                    Authorization().apply {
                        keyParameter =
                            KeyParameter().apply {
                                tag = auth.keyParameter.tag
                                value = KeyParameterValue.integer(replacement)
                            }
                        securityLevel = auth.securityLevel
                    }
                } else {
                    auth
                }
            }
            .toTypedArray()
    }
}
