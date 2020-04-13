package com.live

import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaCodecList
import android.util.Log
import com.live.audio.AudioData
import com.live.video.VideoData
import java.util.concurrent.LinkedBlockingQueue
import kotlin.concurrent.thread

/**
 * Author：Alex
 * Date：2019/10/7
 * Note：
 */
class MediaEncoder {

    companion object {
        var SAVE_FILE_FOR_TEST = false
        val VCODEC = "video/avc" //高级视频编码
    }

    private lateinit var videoEncoderThread: Thread
    private lateinit var audioEncoderThread: Thread
    private var videoEncoderLoop = false
    private var audioEncoderLoop = false

    //队列
    private val videoQueue: LinkedBlockingQueue<VideoData>
    private val audioQueue: LinkedBlockingQueue<AudioData>

    //文件管理
    private lateinit var videoFileManager: FileManager
    private lateinit var audioFileManager: FileManager

    //表示每次传递多少数据为最佳
    private var audioEncodeBuffer = 0
    private var fps = 0

    private lateinit var mediaEncoderCallback: MediaEncoderCallback
    //硬编码
    private lateinit var videoCodec:MediaCodec
    private  lateinit var vCodecInfo: MediaCodecInfo

    init {
        if (SAVE_FILE_FOR_TEST) {
            videoFileManager = FileManager(FileManager.TEST_H264_FILE)
            audioFileManager = FileManager(FileManager.TEST_AAC_FILE)
        }
        videoQueue = LinkedBlockingQueue()
        audioQueue = LinkedBlockingQueue()
        //这里我们初始化音频数据，为什么要初始化音频数据呢？音频数据里面我们做了什么事情？
//        audioEncodeBuffer =
//            LiveNativeManager.encoderAudioInit(LiveConfig.SAMPLE_RATE, LiveConfig.CHANNELS, LiveConfig.BIT_RATE)
    }

    private fun initVideoCodec(){
        
    }

    //查询机器硬编码支持的颜色格式
    private fun chooseColorFormat(){
        vCodecInfo=chooseVideoEncoder(null)

        var matchedColorFormat = 0
        //查询机器上的MediaCodec实现具体支持哪些YUV格式作为输入格式
        val cc = vCodecInfo?.getCapabilitiesForType(VCODEC)
        for (i in cc.colorFormats.indices) {
            val cf = cc.colorFormats[i]
            Log.i("tag", String.format("vencoder %s supports color fomart 0x%x(%d)", vCodecInfo.getName(), cf, cf))

            // choose YUV for h.264, prefer the bigger one.
            // corresponding to the color space transform in onPreviewFrame
            if (cf >= MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar && cf <= MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar) {
                //选大的，如果有大于matchedColorFormat的cf，则选择
                if (cf > matchedColorFormat) {
                    matchedColorFormat = cf
                }
            }
        }
    }

    // choose the video encoder by name.
    private fun chooseVideoEncoder(name: String?): MediaCodecInfo {
        val nbCodecs = MediaCodecList.getCodecCount()
        for (i in 0 until nbCodecs) {
            val mci = MediaCodecList.getCodecInfoAt(i)
            if (!mci.isEncoder) {
                continue
            }

            val types = mci.supportedTypes
            for (j in types.indices) {
                if (types[j].equals(VCODEC, ignoreCase = true)) {
                    Log.i("tag", String.format("vencoder %s types: %s", mci.name, types[j]))
                    if (name == null) {
                        return mci
                    }
                    if (mci.name.contains(name)) {
                        return mci
                    }
                }
            }
        }
        throw java.lang.RuntimeException("can not find videoEncoder")
    }

    fun setAudioEncodeBuffer(size: Int) {
        audioEncodeBuffer = size
        LogUtil.d("音频编码大小=" + size)
    }

    fun setMediaEncoderCallback(callback: MediaEncoderCallback) {
        this.mediaEncoderCallback = callback
    }

    //摄像头的YUV420P数据，put到队列中，生产者模型
    fun putVideoData(videoData: VideoData) {
        try {
            //Log.d("tag","接收到YUV数据="+videoData.with+"////"+videoData.height)
            videoQueue.put(videoData)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    //麦克风PCM音频数据，put到队列中，生产者模型
    fun putAudioData(audioData: AudioData) {
        //Log.d("tag","接收到音频原始数据="+audioData.audioData.size)
        try {
            audioQueue.put(audioData)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun startRecord() {
        SAVE_FILE_FOR_TEST = true
        if (SAVE_FILE_FOR_TEST) {
            videoFileManager = FileManager(FileManager.TEST_H264_FILE)
            audioFileManager = FileManager(FileManager.TEST_AAC_FILE)
        }
    }


    /**
     * 音视频编码回调
     */
    interface MediaEncoderCallback {
        fun receiveEncoderVideoData(videoData: ByteArray, totalLength: Int, segment: IntArray)
        fun receiveEncoderAudioData(audioData: ByteArray, size: Int)
    }

    //开始编码
    fun startEncode() {
        startVideoEncode()
        startAudioEncode2()
    }

    //停止编码
    fun stopEncode() {
        videoEncoderLoop = false
        audioEncoderLoop = false
        videoQueue.clear()
        audioQueue.clear()
//        videoEncoderThread.interrupt()
//        audioEncoderThread.interrupt()
    }

    //开始视频的解码
    private fun startVideoEncode() {
        if (videoEncoderLoop) {
            throw RuntimeException("必须先停止")
        }

        videoEncoderLoop = true
        //编码线程
        videoEncoderThread = thread(start = true) {
            //视频消费者模型，不断从队列中取出视频流来进行h264编码
            while (videoEncoderLoop && !Thread.interrupted()) {

                try {
                    //队列中取视频数据，这里是指一帧图像的数据
                    val videoData = videoQueue.take()
                    fps++ //代表第几帧
                    val outBuffer = ByteArray(videoData.with * videoData.height)
                    val nalLengths = IntArray(10) //保存所有nal单元的大小
                    //对YUV420P进行h264编码，返回一个数据大小为nal单元的个数，里面是编码出来的h264数据，numNals是此次h264编码生成的nal单元数量
                    //buffLength保存每个nal单元的大小
                    val nalsNum = LiveNativeManager.encoderVideoEncode(
                        videoData.videoData,
                        videoData.videoData.size,
                        fps,
                        outBuffer,
                        nalLengths
                    )

                    if (nalsNum > 0) {
                        val segment = IntArray(nalsNum)
                        //segment中保存每个nal单元的大小，将buffLength中每一个nal单元的大小复制到segment中去
                        System.arraycopy(nalLengths, 0, segment, 0, nalsNum)
                        //总长度
                        var totalLength = 0
                        segment.forEach { totalLength += it }
                        //编码后的h264数据
                        val encoderData = ByteArray(totalLength)
                        //outbuffer存储的是编码后的h264数据，在这里做个拷贝
                        System.arraycopy(outBuffer, 0, encoderData, 0, encoderData.size)
                        mediaEncoderCallback?.let { it.receiveEncoderVideoData(encoderData, encoderData.size, segment) }
                        //我们可以把数据在java层保存到文件中，看看我们编码的h264数据是否能播放，h264裸数据可以在VLC播放器中播放
                        if (SAVE_FILE_FOR_TEST) {
                            LogUtil.d("保存视频数据")
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
    private fun startAudioEncode2() {
        if (audioEncoderLoop) {
            throw RuntimeException("必须先停止")
        }
        //开启线程
        audioEncoderLoop = true
        audioEncoderThread = thread(start = true) {
            val outBuffer = ByteArray(1024)
            val inBuffer = ByteArray(audioEncodeBuffer)

            while (audioEncoderLoop && !Thread.interrupted()) {
                try {
                    //从保存队列中获取到音频数据
                    val audioData = audioQueue.take()
                    //我们通过fdk-aac接口获取到了audioEncodeBuffer的数据，即每次编码多少数据为最优
                    //我们每次可以让MIC录取audioEncodeBuffer大小的数据，然后把录取的数据传递到AudioEncoder.cpp中去编码
                    val audioLength = audioData.audioData.size

                    //从audioDataz的0位置开始复制audioLength个字节到haveCopyLength位置开始的inbuffer中
                    System.arraycopy(audioData.audioData, 0, inBuffer, 0, audioLength)

                    //fdk-aac编码PCM裸音频数据，返回可用长度的有效字段
                    val validLength = LiveNativeManager.encoderAudioEncode(
                        inBuffer,
                        audioEncodeBuffer,
                        outBuffer,
                        outBuffer.size
                    )
                    val VALIDLENGTH = validLength
                    if (VALIDLENGTH > 0) {
                        val encoderData = ByteArray(VALIDLENGTH)
                        System.arraycopy(outBuffer, 0, encoderData, 0, VALIDLENGTH)
                        //编码后，把数据抛给rtmp去推流
                        mediaEncoderCallback?.let { it.receiveEncoderAudioData(encoderData, VALIDLENGTH) }
                        //我们可以把Fdk-aac编码后的数据保存到文件中，然后用播放器听一下，音频文件是否编码正确
                        if (SAVE_FILE_FOR_TEST) {
                            audioFileManager.saveFileData(encoderData)
                        }

                    }

                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        }
    }

    /**
     * 开始音频编码
     */
    private fun startAudioEncode() {
        if (audioEncoderLoop) {
            throw RuntimeException("必须先停止")
        }
        //开启线程
        audioEncoderLoop = true
        audioEncoderThread = thread(start = true) {
            val outBuffer = ByteArray(1024)
            var haveCopyLength = 0
            val inBuffer = ByteArray(audioEncodeBuffer)

            while (audioEncoderLoop && !Thread.interrupted()) {
                try {
                    //从保存队列中获取到音频数据
                    val audioData = audioQueue.take()
                    //我们通过fdk-aac接口获取到了audioEncodeBuffer的数据，即每次编码多少数据为最优
                    //这里我这边的手机每次都是返回的4096即4K的数据，其实为了简单点，我们每次可以让
                    //MIC录取4K大小的数据，然后把录取的数据传递到AudioEncoder.cpp中去编码
                    val audioLength = audioData.audioData.size

                    if (haveCopyLength < audioEncodeBuffer) {
                        //从audioDataz的0位置开始复制audioLength个字节到haveCopyLength位置开始的inbuffer中
                        System.arraycopy(audioData.audioData, 0, inBuffer, haveCopyLength, audioLength)
                        haveCopyLength += audioLength
                        //remain用来判断编码的数据是否已经读取完
                        val remain = audioEncodeBuffer - haveCopyLength
                        //如果已经读取到了audioEncodeBuffer长度的数据，则编码PCM裸音频数据
                        if (remain == 0) {
                            //fdk-aac编码PCM裸音频数据，返回可用长度的有效字段
                            val validLength = LiveNativeManager.encoderAudioEncode(
                                inBuffer,
                                audioEncodeBuffer,
                                outBuffer,
                                outBuffer.size
                            )
                            val VALIDLENGTH = validLength
                            if (VALIDLENGTH > 0) {
                                val encoderData = ByteArray(VALIDLENGTH)
                                System.arraycopy(outBuffer, 0, encoderData, 0, VALIDLENGTH)
                                //编码后，把数据抛给rtmp去推流
                                mediaEncoderCallback?.let { it.receiveEncoderAudioData(encoderData, VALIDLENGTH) }
                                //我们可以把Fdk-aac编码后的数据保存到文件中，然后用播放器听一下，音频文件是否编码正确
                                if (SAVE_FILE_FOR_TEST) {
                                    audioFileManager.saveFileData(encoderData)
                                }
                            }
                            haveCopyLength = 0
                        }
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        }
    }

    fun closeSaveFile() {
        if (SAVE_FILE_FOR_TEST) {
            videoFileManager.closeFile()
            audioFileManager.closeFile()
        }
    }
}
























