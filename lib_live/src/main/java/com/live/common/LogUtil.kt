package com.live.common

import android.util.Log

/**
 * Author：Alex
 * Date：2019/7/11
 * Note：
 */
object LogUtil {

    const val LOGTAG = "im_socket"
    const val debugEnabled = true

    private fun getDebugInfo(): String {
        val stack = Throwable().fillInStackTrace()
        val trace = stack.stackTrace
        val n = 2
        return "${trace[n].className} ${trace[n].methodName}():${trace[n].lineNumber} "
    }

    private fun getLogInfoByArray(infos: Array<String>): String {
        val sb = StringBuilder()
        for (info in infos) {
            sb.append(info)
            sb.append(" ")
        }
        return sb.toString()
    }


    fun d(vararg s: String) {
        if (debugEnabled)
            d(
                LOGTAG,
                getDebugInfo() + getLogInfoByArray(
                    s as Array<String>
                )
            )
    }

    fun v(vararg s: String) {
        if (debugEnabled)
            v(
                LOGTAG,
                getDebugInfo() + getLogInfoByArray(
                    s as Array<String>
                )
            )
    }

    fun w(vararg s: String) {
        if (debugEnabled)
            w(
                LOGTAG,
                getDebugInfo() + getLogInfoByArray(
                    s as Array<String>
                )
            )
    }

    fun e(vararg s: String?) {
        if (debugEnabled)
            e(
                LOGTAG,
                getDebugInfo() + getLogInfoByArray(
                    s as Array<String>
                )
            )
    }

    fun i(vararg s: String) {
        if (debugEnabled)
            i(
                LOGTAG,
                getDebugInfo() + getLogInfoByArray(
                    s as Array<String>
                )
            )
    }

    fun e(tr: Throwable) {
        if (debugEnabled)
            Log.e(LOGTAG, getDebugInfo() + getDebugInfo(), tr)
    }


    private fun d(name: String, log: String) {
        System.out.println("$name : $log")
    }

    private fun v(name: String, log: String) {
        System.out.println("$name : $log")
    }

    private fun i(name: String, log: String) {
        System.out.println("$name : $log")
    }

    private fun e(name: String, log: String) {
        System.out.println("$name : $log")
    }

    private fun w(name: String, log: String) {
        System.out.println("$name : $log")
    }
}