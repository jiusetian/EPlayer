package com.eplayer.common

import android.content.Context
import android.database.Cursor
import android.provider.MediaStore
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

    fun getScreenHeight(context: Context): Int {
        val wm = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager

        val outMetrics = DisplayMetrics()

        wm.defaultDisplay.getMetrics(outMetrics)

        return outMetrics.heightPixels
    }

    fun getVideoList(context: Context): List<String> {
        val list = ArrayList<String>()

        if (this != null) {
            val cursor: Cursor = context.getContentResolver().query(
                MediaStore.Video.Media.EXTERNAL_CONTENT_URI, null, null,
                null, null
            )
            if (cursor != null) {
                while (cursor.moveToNext()) {
                    val path: String = cursor
                        .getString(cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA))
                    list.add(path)
                }
                cursor.close()
            }
        }
        return list
    }
}