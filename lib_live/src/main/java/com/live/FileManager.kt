package com.live

import java.io.File
import java.io.FileOutputStream

/**
 * Author：Alex
 * Date：2019/9/21
 * Note：文件管理
 */
class FileManager(fileName: String) {

    companion object {
        const val TEST_PCM_FILE = "/sdcard/123.pcm"
        const val TEST_WAV_FILE = "/sdcard/123.wav"
        const val TEST_YUV_FILE = "/sdcard/123.yuv" //YUV存储路径
        const val TEST_H264_FILE = "/sdcard/123.h264"
        const val TEST_AAC_FILE = "/sdcard/123.aac"
    }

    private lateinit var fileOutputStream: FileOutputStream

    //初始化
    init {
        try {
            val file = File(fileName)
            if (file.exists()) {
                file.delete()
            } else {
                file.createNewFile()
            }
            fileOutputStream = FileOutputStream(file)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun saveFileData(data: ByteArray, offset: Int, length: Int) {

        try {
            fileOutputStream.write(data, offset, length)
            fileOutputStream.flush()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun saveFileData(data: ByteArray) {
        try {
            fileOutputStream.write(data)
            fileOutputStream.flush()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun closeFile() {
        try {
            fileOutputStream.close()
        } catch (e: Exception) {
            e.printStackTrace()
        } finally {
            fileOutputStream.close()
        }
    }


}