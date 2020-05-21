package com.live

import android.content.Context
import com.live.audio.AudioData
import com.live.audio.AudioGatherManager
import com.live.common.FileManager
import com.live.common.LiveInterfaces
import com.live.video.CameraSurface
import com.live.video.VideoData
import com.live.video.VideoGatherManager
import java.util.*
import java.util.concurrent.LinkedBlockingQueue
import kotlin.concurrent.thread

/**
 * Author：Alex
 * Date：2019/10/7
 * Note：
 */
class MediaEncoder(val context: Context, val cameraSurface: CameraSurface) :
    LiveInterfaces {

    companion object {
        var SAVE_FILE_FOR_TEST = false
        val VCODEC = "video/avc" //高级视频编码
    }

    // 音视频编码线程
    private  var videoEncoderThread: Thread?=null
    private  var audioEncoderThread: Thread?=null

    // 线程控制
    private var videoEncoderLoop = true
    private var audioEncoderLoop = true


    // 管理音视频数据的获取
    private lateinit var videoGatherManager: VideoGatherManager
    private lateinit var audioGatherManager: AudioGatherManager

    // 队列
    private val videoQueue: LinkedBlockingQueue<VideoData>
    private val audioQueue: LinkedBlockingQueue<AudioData>

    // 文件管理
    private lateinit var videoFileManager: FileManager
    private lateinit var audioFileManager: FileManager

    private var fps = 0

    // 编码数据的回调
    private lateinit var mediaEncoderCallback: MediaEncoderCallback

    init {
        // 文件保存
        if (SAVE_FILE_FOR_TEST) {
            videoFileManager =
                FileManager(FileManager.TEST_H264_FILE)
            audioFileManager =
                FileManager(FileManager.TEST_AAC_FILE)
        }
        // 音视频数据队列
        videoQueue = LinkedBlockingQueue()
        audioQueue = LinkedBlockingQueue()
    }

    fun onMediaEncoderCallback(callback: MediaEncoderCallback) {
        mediaEncoderCallback = callback
    }


    override fun init() {
        // 视频采集器的初始化
        videoGatherManager = VideoGatherManager(cameraSurface, context)
        videoGatherManager.init()

        // 回调视频压缩数据
        videoGatherManager.onCompressDataListener { data, width, height ->
            val videoData = VideoData(data, width, height)
            // 保存视频数据
            videoQueue.put(videoData)
        }

        // 音频采集器的初始化
        audioGatherManager = AudioGatherManager()
        audioGatherManager.init()

        // 回调音频pcm数据
        audioGatherManager.onAudioDataListener {
                val audioData = AudioData(it)
                // 保存音频数据
                audioQueue.put(audioData)
        }
    }

    fun switchCamera() {
        videoGatherManager.switchCamera()
    }

    override fun start() {
        // 开始音视频的采集
        videoGatherManager.start()
        audioGatherManager.start()
        // 开始音视频编码线程
        startVideoEncoder()
        startAudioEncode()
    }

    override fun stop() {
        videoGatherManager.stop()
        audioGatherManager.stop()

        videoEncoderThread?.interrupt()
        audioEncoderThread?.interrupt()
        videoEncoderLoop = false
        audioEncoderLoop = false
        // 清空数据
        videoQueue.clear()
        audioQueue.clear()

        if (SAVE_FILE_FOR_TEST) {
            videoFileManager.closeFile()
            audioFileManager.closeFile()
        }
    }

    override fun destrory() {
        TODO("Not yet implemented")
    }

    override fun pause() {
        videoGatherManager.pause()
        audioGatherManager.pause()
    }

    override fun resume() {
        videoGatherManager.resume()
        audioGatherManager.resume()
    }

    // 开始视频的解码
    private fun startVideoEncoder() {
        // 编码线程
        videoEncoderThread = thread(start = true) {

            while (videoEncoderLoop && !Thread.interrupted()) {
                try {
                    // 队列中取视频数据，这里是指一帧图像的数据
                    val videoData = videoQueue.take()
                    fps++ // 代表第几帧

                    // 保存编码输出数据
                    val outBuffer = ByteArray(videoData.with * videoData.height)
                    val nalLengths = IntArray(10) // 保存nal单元的大小

                    // 对YUV420P进行h264编码，返回数据为nal单元的个数，编码出来的是 h264数据，nalsNum是此次编码生成的nal单元数量
                    val nalsNum = LiveNativeApi.videoEncode(
                        videoData.videoData,
                        videoData.videoData.size,
                        fps,
                        outBuffer,
                        nalLengths
                    )

                    if (nalsNum > 0) {
                        // 所有nal单元大小
                        val nalsSize = IntArray(nalsNum)
                        System.arraycopy(nalLengths, 0, nalsSize, 0, nalsNum)

                        // 总长度
                        var totalLength = 0
                        nalsSize.forEach { totalLength += it }

                        // 编码后的h264数据
                        val encoderData = ByteArray(totalLength)
                        // outbuffer存储的是编码后的h264数据，做个拷贝
                        System.arraycopy(outBuffer, 0, encoderData, 0, encoderData.size)
                        mediaEncoderCallback?.let {
                            it.receiveEncoderVideoData(
                                encoderData,
                                encoderData.size,
                                nalsSize
                            )
                        }

                        // 保存 h264数据
                        if (SAVE_FILE_FOR_TEST) {
                            videoFileManager.saveFileData(encoderData)
                        }
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                }

            }
        }
    }

    /**
     * 开始音频编码，音频音频每次录制的数据大小跟编码器要求的数据大小相等，所以这里可以简略一点
     */
    private fun startAudioEncode() {

        audioEncoderThread = thread(start = true) {
            val outBuffer = ByteArray(1024)

            while (audioEncoderLoop && !Thread.interrupted()) {
                try {
                    // 从保存队列中获取到音频数据
                    val audioData = audioQueue.take()
                    // 我们通过fdk-aac接口获取到了audioEncodeBuffer的数据，即每次编码多少数据为最优
                    // 我们每次可以让MIC录取 audioEncodeBuffer大小的数据，然后把录取的数据传递到AudioEncoder.cpp中去编码
                    val audioLength = audioData.audioData.size
                    val inBuffer = ByteArray(audioLength)
                    // 从audioDataz的0位置开始复制audioLength个字节到 haveCopyLength位置开始的 inbuffer中
                    System.arraycopy(audioData.audioData, 0, inBuffer, 0, audioLength)

                    // 发送给 NDK层的 fdk-aac编码 PCM裸音频数据，返回可用长度的有效字段
                    val validLength = LiveNativeApi.audioEncode(
                        inBuffer,
                        audioLength,
                        outBuffer,
                        outBuffer.size
                    )

                    // 编码成功
                    if (validLength > 0) {
                        val encoderData = ByteArray(validLength)
                        // 复制数据，因为outBuffer要重新赋值的，所以要复制
                        System.arraycopy(outBuffer, 0, encoderData, 0, validLength)
                        // 回调音频编码数据
                        mediaEncoderCallback?.let { it.receiveEncoderAudioData(encoderData, validLength) }
                        // 保存
                        if (SAVE_FILE_FOR_TEST) {
                            audioFileManager.saveFileData(encoderData)
                        }
                    }

                } catch (e: Exception) {
                    e.printStackTrace()
                }
                Arrays.fill(outBuffer, 0)
            }
        }
    }

    /**
     * 音视频编码回调接口
     */
    interface MediaEncoderCallback {
        fun receiveEncoderVideoData(videoData: ByteArray, totalLength: Int, segment: IntArray)
        fun receiveEncoderAudioData(audioData: ByteArray, size: Int)
    }
}
























