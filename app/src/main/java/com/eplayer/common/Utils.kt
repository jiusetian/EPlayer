package com.eplayer.common

import android.content.Context
import android.util.DisplayMetrics
import android.view.WindowManager


/**
 * Author：Alex
 * Date：2019/10/31
 * Note：
 */
object Utils {

    fun getScreenWidth(context: Context): Int {
        val wm = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager

        val outMetrics = DisplayMetrics()

        wm.defaultDisplay.getMetrics(outMetrics)

        return outMetrics.widthPixels
    }

    fun getScreenHeight(context: Context):Int{
        val wm = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager

        val outMetrics = DisplayMetrics()

        wm.defaultDisplay.getMetrics(outMetrics)

        return outMetrics.heightPixels
    }
}