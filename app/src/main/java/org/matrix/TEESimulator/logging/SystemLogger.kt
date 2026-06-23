package org.matrix.TEESimulator.logging

import android.util.Log
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicLong
import org.matrix.TEESimulator.BuildConfig

/**
 * A centralized logging utility for the TEESimulator application. This object provides a consistent
 * logging tag and format for all application logs, making it easier to filter and debug in Logcat.
 *
 * Includes a rate limiter that caps logd syscalls during binder stress to prevent thread pool
 * contention. The first [RATE_LIMIT_BURST] messages per [RATE_LIMIT_WINDOW_MS] window are logged
 * normally; subsequent messages are suppressed and a summary is emitted when the window resets.
 */
object SystemLogger {
    @PublishedApi internal const val TAG = "TEESimulator"

    @PublishedApi internal val isDebugBuild = BuildConfig.DEBUG

    // Rate limiter: allow BURST messages per WINDOW, then suppress until window resets.
    private const val RATE_LIMIT_BURST = 15
    private const val RATE_LIMIT_WINDOW_MS = 1000L
    private val windowStart = AtomicLong(System.currentTimeMillis())
    private val windowCount = AtomicInteger(0)
    private val suppressedCount = AtomicInteger(0)

    /**
     * Returns true if this message should be emitted. Resets the window if expired
     * and emits a suppression summary for the previous window.
     */
    @PublishedApi internal fun acquireLogPermit(): Boolean {
        val now = System.currentTimeMillis()
        val start = windowStart.get()
        if (now - start > RATE_LIMIT_WINDOW_MS) {
            // Window expired: reset and emit suppression summary if needed.
            if (windowStart.compareAndSet(start, now)) {
                val suppressed = suppressedCount.getAndSet(0)
                windowCount.set(1) // this call counts as #1 in the new window
                if (suppressed > 0) {
                    Log.i(TAG, "[rate-limit] suppressed $suppressed log messages in previous window")
                }
                return true
            }
        }
        val count = windowCount.incrementAndGet()
        if (count <= RATE_LIMIT_BURST) return true
        suppressedCount.incrementAndGet()
        return false
    }

    /**
     * Logs a debug message. Use this for fine-grained information that is useful for debugging.
     */
    fun debug(message: String) {
        if (!isDebugBuild) return
        if (!acquireLogPermit()) return
        Log.d(TAG, message)
    }

    /** Lazy debug: lambda only evaluates if message will be logged. */
    inline fun debug(message: () -> String) {
        if (!isDebugBuild) return
        if (!acquireLogPermit()) return
        Log.d(TAG, message())
    }

    /**
     * Logs an informational message. Use this to report major application lifecycle events.
     */
    fun info(message: String) {
        if (!acquireLogPermit()) return
        Log.i(TAG, message)
    }

    /** Lazy info: lambda only evaluates if message will be logged. */
    inline fun info(message: () -> String) {
        if (!acquireLogPermit()) return
        Log.i(TAG, message())
    }

    /**
     * Logs a warning message. Warnings are never rate-limited.
     */
    fun warning(message: String, throwable: Throwable? = null) {
        if (throwable != null) {
            Log.w(TAG, message, throwable)
        } else {
            Log.w(TAG, message)
        }
    }

    /**
     * Logs an error message. Errors are never rate-limited.
     */
    fun error(message: String, throwable: Throwable? = null) {
        if (throwable != null) {
            Log.e(TAG, message, throwable)
        } else {
            Log.e(TAG, message)
        }
    }

    /**
     * Logs a verbose message. This level is for highly detailed logs that are generally not needed
     * unless tracking a very specific issue.
     */
    fun verbose(message: String) {
        if (!isDebugBuild) return
        if (!acquireLogPermit()) return
        Log.v(TAG, message)
    }

    /** Lazy verbose: lambda only evaluates if message will be logged. */
    inline fun verbose(message: () -> String) {
        if (!isDebugBuild) return
        if (!acquireLogPermit()) return
        Log.v(TAG, message())
    }

    inline fun trace(message: () -> String) {
        if (!isDebugBuild) return
        Log.w(TAG, message())
    }
}
