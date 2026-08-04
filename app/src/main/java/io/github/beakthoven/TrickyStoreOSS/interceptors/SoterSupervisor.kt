/*
 * Soter process supervisor — ported from TEESimulator-RS 307, adapted to V3.
 * Binds com.tencent.soter.soterserver, injects libfateh7.so, registers the
 * SoterInterceptor forge, and re-binds on process death.
 */

package io.github.beakthoven.TrickyStoreOSS.interceptors

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.util.Log
import io.github.beakthoven.TrickyStoreOSS.logging.TAG
import java.util.concurrent.Executor
import java.util.concurrent.atomic.AtomicBoolean

object SoterSupervisor {

    private const val SOTER_PACKAGE = "com.tencent.soter.soterserver"
    private const val INJECTION_COMMAND =
        "exec ./inject `pidof com.tencent.soter.soterserver` libfateh7.so entry"
    private const val REBIND_DELAY_MS = 1000L
    private const val REBIND_MAX_MS = 30_000L

    private val started = AtomicBoolean(false)
    private var rebindDelay = REBIND_DELAY_MS
    private lateinit var context: Context
    private lateinit var handler: Handler
    private val executor = Executor { command -> handler.post(command) }

    fun start(context: Context) {
        if (!started.compareAndSet(false, true)) return
        this.context = context
        handler = Handler(HandlerThread("soter-supervisor").apply { start() }.looper)
        handler.post { bind() }
    }

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            Log.d(TAG, "SOTER service connected; mounting forge")
            service?.let(::mount)
        }
        override fun onServiceDisconnected(name: ComponentName?) {
            Log.d(TAG, "SOTER disconnected; rebinding")
            scheduleRetry()
        }
        override fun onBindingDied(name: ComponentName?) {
            Log.d(TAG, "SOTER binding died; rebinding")
            scheduleRetry()
        }
        override fun onNullBinding(name: ComponentName?) {
            Log.d(TAG, "SOTER onBind null; rebinding")
            scheduleRetry()
        }
    }

    private fun bind() {
        val intent = Intent(SoterInterceptor.DESCRIPTOR).setPackage(SOTER_PACKAGE)
        val bound = runCatching {
            context.bindService(intent, Context.BIND_AUTO_CREATE, executor, connection)
        }.getOrElse {
            Log.d(TAG, "SOTER bindService threw: $it")
            false
        }
        if (!bound) {
            Log.d(TAG, "SOTER bindService false; retrying")
            scheduleRetry()
        }
    }

    private fun rebind() {
        runCatching { context.unbindService(connection) }
        bind()
    }

    private fun scheduleRetry() {
        val delay = rebindDelay
        rebindDelay = (rebindDelay * 2).coerceAtMost(REBIND_MAX_MS)
        handler.postDelayed({ rebind() }, delay)
    }

    private fun mount(soterBinder: IBinder) {
        var backdoor = BinderInterceptor.getBinderBackdoor(soterBinder)
        if (backdoor == null) {
            Log.d(TAG, "SOTER backdoor absent; injecting libfateh7.so")
            if (!injectLibrary()) {
                scheduleRetry()
                return
            }
            backdoor = BinderInterceptor.getBinderBackdoor(soterBinder)
        }
        if (backdoor == null) {
            Log.d(TAG, "SOTER backdoor handshake failed; re-bind")
            scheduleRetry()
            return
        }
        BinderInterceptor.registerBinderInterceptor(backdoor, soterBinder, SoterInterceptor)
        rebindDelay = REBIND_DELAY_MS
        Log.d(TAG, "SOTER forge mounted; handshake ok")
    }

    private fun injectLibrary(): Boolean =
        runCatching {
            Runtime.getRuntime().exec(arrayOf("/system/bin/sh", "-c", INJECTION_COMMAND)).waitFor() == 0
        }.getOrElse {
            Log.d(TAG, "SOTER inject exec failed: $it")
            false
        }
}
