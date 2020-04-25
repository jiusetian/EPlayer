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

    fun getScreenHeight(context: Context):Int{
        val wm = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager

        val outMetrics = DisplayMetrics()

        wm.defaultDisplay.getMetrics(outMetrics)

        return outMetrics.heightPixels
    }

    fun getList(context: Context): List<String> {
        var list: MutableList<String>
        list = ArrayList()

        if (this != null) {
            val cursor: Cursor = context.getContentResolver().query(
                MediaStore.Video.Media.EXTERNAL_CONTENT_URI, null, null,
                null, null
            )
            if (cursor != null) {

                while (cursor.moveToNext()) {

//                    val id: Int = cursor.getInt(
//                        cursor
//                            .getColumnIndexOrThrow(MediaStore.Video.Media._ID)
//                    )
//                    val title: String = cursor
//                        .getString(
//                            cursor
//                                .getColumnIndexOrThrow(MediaStore.Video.Media.TITLE)
//                        )
//
////                    val album: String = cursor.getString(
////                            cursor.getColumnIndexOrThrow(MediaStore.Video.Media.ALBUM))
//
//                    val artist: String = cursor
//                        .getString(
//                            cursor
//                                .getColumnIndexOrThrow(MediaStore.Video.Media.ARTIST)
//                        )
//                    val displayName: String = cursor
//                        .getString(
//                            cursor
//                                .getColumnIndexOrThrow(MediaStore.Video.Media.DISPLAY_NAME)
//                        )
//                    val mimeType: String = cursor
//                        .getString(
//                            cursor
//                                .getColumnIndexOrThrow(MediaStore.Video.Media.MIME_TYPE)
//                        )
                    val path: String = cursor
                        .getString(
                            cursor.getColumnIndexOrThrow(MediaStore.Video.Media.DATA)
                        )

//                    val size: Long = cursor
//                        .getLong(
//                            cursor
//                                .getColumnIndexOrThrow(MediaStore.Video.Media.SIZE)
//                        )

                    list.add(path)
                }
                cursor.close()
            }
        }

        return list
    }
}