package com.live

import android.content.Context
import com.live.audio.AudioData
import com.live.audio.AudioGatherManager
import com.live.muxer.SrsFlvMuxer
import com.live.simplertmp.RtmpHandler
import com.live.video.CameraSurface
import com.live.video.VideoData
import com.live.video.VideoGatherManager
import java.util.concurrent.LinkedBlockingQueue
import kotlin.concurrent.thread

/**
 * Author：Alex
 * Date：2019/10/8
 * Note：
 */
class MediaPublisher(val context: Context, val cameraSurface: CameraSurface, val rtmpUrl: String) {

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

    private lateinit var videoGatherManager: VideoGatherManager
    private lateinit var audioGatherManager: AudioGatherManager
    private val mediaEncoder: MediaEncoder
    private lateinit var flvMuxer: SrsFlvMuxer
    private var isPublish = true

    private val runnables = LinkedBlockingQueue<Runnable>()
    private val rtmpThread: Thread
    private var loop = false

    private var videoID = 0L
    private var audioID = 0L
    private var isSendAudioSpec = false //是否已经发送了音频头信息

    init {
        mediaEncoder = MediaEncoder()
        //音视频编码的数据回调
        mediaEncoder.setMediaEncoderCallback(object : MediaEncoder.MediaEncoderCallback {

            override fun receiveEncoderVideoData(videoData: ByteArray, totalLength: Int, segment: IntArray) {
                //Log.d("tag","接收编码后的视频数据="+videoData.size)
                onEncoderVideoData(videoData, totalLength, segment)
            }

            override fun receiveEncoderAudioData(audioData: ByteArray, size: Int) {
                //Log.d("tag","接收到编码后的音频数据="+audioData.size)
                onEncoderAudioData(audioData, size)
            }

        })

        rtmpThread = thread(start = true, name = "publish_thread") {
            loop = true
            while (loop && !Thread.interrupted()) {
                try {
                    runnables.take().run()
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        }
        //初始化rtmp连接
        //initRtmp()
        //初始化音视频相关
        initVideoGatherManager(context, cameraSurface)
        initAudioGatherManager()
    }

    //获取
    fun getVideoGatherManager() = videoGatherManager

    private fun initVideoGatherManager(context: Context, cameraSurface: CameraSurface) {
        this.videoGatherManager = VideoGatherManager(cameraSurface, context)
        videoGatherManager.setYuvDataListener { data, width, height ->
            if (isPublish) {
                //LogUtil.d("给编码器的宽高=" + width + "////" + height)
                val videoData = VideoData(data, width, height)
                //交给编码器去编码
                mediaEncoder.putVideoData(videoData)
            }
        }
    }

    private fun initAudioGatherManager() {
        audioGatherManager = AudioGatherManager()
        audioGatherManager.setAudioDataListener {
            if (isPublish) {
                val audioData = AudioData(it)
                //交给音视频解码器去解码
                mediaEncoder.putAudioData(audioData)
            }
        }
    }

    //设置rtmphandler
    fun setRtmpHandler(handler: RtmpHandler) {
        flvMuxer = SrsFlvMuxer(handler)
    }

    //media编码相关
    fun startMediaEncoder() {
        mediaEncoder.startEncode()
    }

    fun stopMediaEncoder() {
        mediaEncoder.stopEncode()
    }

    fun getMediaEncoder(): MediaEncoder {
        return mediaEncoder
    }

    //停止推流
    fun stopPublish() {
        loop = false
        Thread.interrupted()
        runnables.clear()
    }

    //音频相关
    fun startAudioGather() {
        val bufferSize = audioGatherManager.initAudio()
        mediaEncoder.setAudioEncodeBuffer(bufferSize)
        audioGatherManager.startAudioGather()
    }

    fun stopAudioGather() {
        audioGatherManager.stopAudioGather()
    }

    //视频相关
    fun startVideoGather() {
        videoGatherManager.startVideoGather()
    }

    fun stopVideoGather() {
        videoGatherManager.stopVideoGather()
    }

    //rtmp相关
    fun initRtmp() {
        //初始化rtmp
        val runnable = Runnable { LiveNativeManager.initRtmpData(rtmpUrl) }
        putRunnable(runnable)
    }

    fun relaseRtmp() {
        val runnable = Runnable { LiveNativeManager.releaseRtmp() }
        putRunnable(runnable)
    }

    //释放io流
    fun releaseIOStream() {
        videoGatherManager.releaseIOStream()
        audioGatherManager.realeaseStream()
        mediaEncoder.closeSaveFile()
    }


    private fun putRunnable(runnable: Runnable) {
        try {
            runnables.put(runnable)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    /**
     * 接收编码好的视频数据
     */
    private fun onEncoderVideoData(encoderVideoData: ByteArray, totalLength: Int, segment: IntArray) {
        var spsLen = 0
        var ppsLen = 0
        var sps: ByteArray = byteArrayOf()
        var pps: ByteArray
        var haveCopy = 0

        //segment为C++传递上来的数组，当为SPS，PPS的时候，视频NALU数组大于1，其它时候等于1
        segment.forEach {

            //nal长度
            val segmentLength = it
            val segmentByte = ByteArray(segmentLength)
            //复制nalu数据到segmentByte，包括起始码
            System.arraycopy(encoderVideoData, haveCopy, segmentByte, 0, segmentLength)
            haveCopy += segmentLength
            //起始码
            var offset = 4
            if (segmentByte[0].toInt() == 0x00 && segmentByte[1].toInt() == 0x00 && segmentByte[2].toInt() == 0x01) {
                offset = 3
            }
            //获取nal的类型，nal单元的第一个字节为header信息，有nal的类型值
            //LogUtil.d("是否包含起始码3="+isContainStartCode(segmentByte))
            val type = segmentByte[offset].toInt().and(0x1f)
            //获取到NALU的type，SPS，PPS，SEI，还是关键帧
            if (type == NAL_SPS) {
                //减去起始码长度
                spsLen = segmentLength - offset
                sps = ByteArray(spsLen)
                System.arraycopy(segmentByte, offset, sps, 0, spsLen)
                //LogUtil.d("sps的类型值"+sps[0].toInt()+"...."+sps[1].toInt())
                //LogUtil.d("是否包含起始码1="+isContainStartCode(sps))
            } else if (type == NAL_PPS) {
                ppsLen = segmentLength - offset
                pps = ByteArray(ppsLen)
                System.arraycopy(segmentByte, offset, pps, 0, ppsLen)
                //LogUtil.d("是否包含起始码2="+isContainStartCode(pps))
                //LogUtil.d("pps的类型值"+pps[0].toInt()+"...."+pps[1].toInt())
                sendVideoSPSAndPPS(sps, spsLen, pps, ppsLen, 0)
            } else {
                sendVideoData(segmentByte, segmentLength, videoID++)
            }
        }
    }

    //是否包含起始码
    private fun isContainStartCode(dataArray: ByteArray): Boolean {
        var isContain = false

        for (i in 0 until dataArray.size - 4) {
            // not match.
            //只要相邻两个byte有其中一个不是0x00，就进入下一个循环，也就是说只有两个都是0x00，才往下执行
            if (dataArray.get(i).toInt() != 0x00 || dataArray.get(i + 1).toInt() != 0x00) {
                continue
            }
            // match N[00] 00 00 01, where N>=0
            if (dataArray.get(i + 2).toInt() == 0x01) {
                isContain = true
                break
            }
            // match N[00] 00 00 00 01, where N>=0
            if (dataArray.get(i + 2).toInt() == 0x00 && dataArray.get(i + 3).toInt() == 0x01) {
                isContain = true
                //Log.d(TAG, "searchAnnexb: 我的值是="+bb.position()+"....."+annexb.nb_start_code);
                break
            }
        }
        return isContain
    }


    /**
     * 接收编码好的音频数据
     */
    private fun onEncoderAudioData(audioData: ByteArray, size: Int) {
        //只有第一次发送音频数据的时候，才先发送一个音频同步包
        if (!isSendAudioSpec) {
            sendAudioSpec(0)
            isSendAudioSpec = true
        }
        sendAudioData(audioData, size, audioID++)
    }

    //发送视频头信息
    private fun sendVideoSPSAndPPS(sps: ByteArray, spslen: Int, pps: ByteArray, ppslen: Int, timeStamp: Long) {
        //LogUtil.d("发送视频头信息")
        val runnable = Runnable { LiveNativeManager.sendRtmpVideoSpsPPS(sps, spslen, pps, ppslen, timeStamp) }
        putRunnable(runnable)
    }

    //发送视频数据
    private fun sendVideoData(data: ByteArray, datalen: Int, timeStamp: Long) {
        val runnable = Runnable { LiveNativeManager.sendRtmpVideoData(data, datalen, timeStamp) }
        putRunnable(runnable)
    }

    //发送音频头信息
    private fun sendAudioSpec(timeStamp: Long) {
        val runnable = Runnable { LiveNativeManager.sendRtmpAudioSpec(timeStamp) }
        putRunnable(runnable)
    }

    //发送音频数据
    private fun sendAudioData(data: ByteArray, datalen: Int, timeStamp: Long) {
        val runnable = Runnable { LiveNativeManager.sendRtmpAudioData(data, datalen, timeStamp) }
        putRunnable(runnable)
    }

}