package io.github.beakthoven.TrickyStoreOSS

/**
 * Obfuscated config directory resolver.
 *
 * The real path is stored XOR-encrypted so it does not appear as a
 * plaintext string in the compiled dex. It is decoded once at runtime.
 */
internal object ConfigPaths {
    // XOR-encrypted config base (key 0x5A)
    private val enc = byteArrayOf(
        117, 62, 59, 46, 59, 117, 55, 51, 41, 57, 117,
        46, 50, 63, 5, 52, 63, 34, 46, 5, 34, 34
    )
    private const val KEY = 0x5A.toByte()

    val base: String by lazy {
        val out = ByteArray(enc.size)
        for (i in enc.indices) {
            out[i] = (enc[i].toInt() xor KEY.toInt()).toByte()
        }
        String(out)
    }

    // convenience helpers
    fun sub(name: String): String = "$base/$name"
}
