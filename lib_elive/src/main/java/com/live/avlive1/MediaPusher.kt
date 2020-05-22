package com.live.avlive1

import android.content.Context
import com.live.LiveNativeApi
import com.live.common.LiveInterfaces
import com.live.avlive1.video.CameraSurface
import java.util.concurrent.LinkedBlockingQueue
import kotlin.concurrent.thread

/**
 * Author：Alex
 * Date：2019/10/8
 * Note：
 */
class MediaPusher(val context: Context, val cameraSurface: CameraSurface, val rtmpUrl: String) : LiveInterfaces,
    MediaEncoder.MediaEncoderCallback {

    companion object {
        const val NAL_UNKNOWN = 0
        const val NAL_SLICE = 1 /* 非关键帧 */
        const val NAL_SLICE_DPA = 2
        const val NAL_SLICE_DPB = 3
        const val NAL_SLICE_DPC = 4
        const val NAL_SLICE_IDR = 5 /* 关键帧 */
        const val NAL_SEI = 6
        const val NAL_SPS = 7 /* SPS帧 */
        const val NAL_PPS = 8 /* PPS帧 */
        const val NAL_AUD = 9
        const val NAL_FILLER = 12
    }

    // 编码器
    private val mediaEncoder: MediaEncoder

    // 任务队列
    private val tasks = LinkedBlockingQueue<Runnable>()

    // 推流线程
    private var rtmpThread: Thread? = null
    private var loop = true

    private var videoID = 0L
    private var audioID = 0L

    // 是否已发送音频头信息
    private var isSendAudioSpec = false

    init {
        mediaEncoder = MediaEncoder(context, cameraSurface)
    }

    override fun init() {
        mediaEncoder.init()
        mediaEncoder.onMediaEncoderCallback(this)
        //初始化 rtmp
        thread {
            LiveNativeApi.initRtmpData(rtmpUrl)
        }
    }

    override fun start() {
        mediaEncoder.start()
        // rtmp推流线程
        rtmpThread = thread {
            loop=true

            while (loop && !Thread.interrupted()) {
                try {
                    // 执行 runnable
                    tasks.take().run()
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        }
    }

    // 切换摄像头
    fun switchCamera() {
        mediaEncoder.switchCamera()
    }

    // 改变摄像机方向
    fun changeCarmeraOrientation(): Int {
        return cameraSurface.changeCarmeraOrientation()
    }

    override fun stop() {
        mediaEncoder.stop()
        loop = false
        rtmpThread?.interrupt()
        tasks.clear()

        rtmpThread?.join()
        LiveNativeApi.releaseRtmp()
    }

    override fun destrory() {
        TODO("Not yet implemented")
    }

    override fun pause() {
        mediaEncoder.pause()
    }

    override fun resume() {
        mediaEncoder.resume()
    }

    fun openCamera() {
        cameraSurface.openCamera()
    }

    fun closeCamera() {
        cameraSurface.releaseCamera()
    }

    // 添加任务
    private fun addTask(runnable: Runnable) {
        try {
            tasks.put(runnable)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    // 音视频编码数据的回调
    override fun receiveEncoderVideoData(videoData: ByteArray, totalLength: Int, nalsSize: IntArray) {
        dealVideoEncodeData(videoData, totalLength, nalsSize)
    }

    override fun receiveEncoderAudioData(audioData: ByteArray, size: Int) {
        dealAudioEncodeData(audioData, size)
    }

    /**
     * 接收编码好的视频数据
     */
    private fun dealVideoEncodeData(encoderVideoData: ByteArray, totalLength: Int, nalSizes: IntArray) {

        var spsBytes: ByteArray = byteArrayOf()
        var ppsBytes: ByteArray
        var haveCopy = 0

        // segment为 C++传递上来的数组，当为SPS，PPS的时候，视频 NALU数组大于1，其它时候等于1
        nalSizes.forEach {

            // 当前nal长度
            val nalLen = it
            // 保存 nal数据
            val nalBytes = ByteArray(nalLen)

            // 复制 nal数据到segmentByte，包括起始码
            System.arraycopy(encoderVideoData, haveCopy, nalBytes, 0, nalLen)
            haveCopy += nalLen
            // 起始码
            var offset = 4
            if (nalBytes[0].toInt() == 0x00 && nalBytes[1].toInt() == 0x00 && nalBytes[2].toInt() == 0x01) {
                offset = 3
            }
            // nal的类型
            val type = nalBytes[offset].toInt().and(0x1f)

            // 获取到 NALU的 type，SPS，PPS，SEI，还是关键帧
            if (type == NAL_SPS) {
                // 减去起始码长度
                val spsLen = nalLen - offset
                spsBytes = ByteArray(spsLen)
                // sps数据
                System.arraycopy(nalBytes, offset, spsBytes, 0, spsLen)
            } else if (type == NAL_PPS) {
                val ppsLen = nalLen - offset
                ppsBytes = ByteArray(ppsLen)
                // pps数据
                System.arraycopy(nalBytes, offset, ppsBytes, 0, ppsLen)

                // 发送sps和pps
                sendVideoSPSAndPPS(spsBytes, ppsBytes, 0)
            } else {
                // 发送视频数据
                sendVideoData(nalBytes, nalLen, videoID++)
            }

        }
    }


    /**
     * 接收编码好的音频数据
     */
    private fun dealAudioEncodeData(audioData: ByteArray, size: Int) {
        // 第一次发送音频数据的时候，先发送一个音频同步包
        if (!isSendAudioSpec) {
            sendAudioSpec(0)
            isSendAudioSpec = true
        }
        sendAudioData(audioData, size, audioID++)
    }

    // 发送视频头信息
    private fun sendVideoSPSAndPPS(sps: ByteArray, pps: ByteArray, timeStamp: Long) {
        val runnable = Runnable {
            LiveNativeApi.sendRtmpVideoSpsPPS(
                sps,
                sps.size,
                pps,
                pps.size,
                timeStamp
            )
        }
        addTask(runnable)
    }

    // 发送视频数据
    private fun sendVideoData(data: ByteArray, datalen: Int, timeStamp: Long) {
        val runnable = Runnable {
            LiveNativeApi.sendRtmpVideoData(
                data,
                datalen,
                timeStamp
            )
        }
        addTask(runnable)
    }

    // 发送音频头信息
    private fun sendAudioSpec(timeStamp: Long) {
        val runnable = Runnable { LiveNativeApi.sendRtmpAudioSpec(timeStamp) }
        addTask(runnable)
    }

    // 发送音频数据
    private fun sendAudioData(data: ByteArray, datalen: Int, timeStamp: Long) {
        val runnable = Runnable {
            LiveNativeApi.sendRtmpAudioData(
                data,
                datalen,
                timeStamp
            )
        }
        addTask(runnable)
    }

}