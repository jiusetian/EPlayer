package com.live.audio

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.media.audiofx.AcousticEchoCanceler
import android.media.audiofx.AutomaticGainControl
import com.live.FileManager
import com.live.LiveConfig
import com.live.LiveNativeManager
import com.live.LogUtil
import java.util.*
import kotlin.concurrent.thread


/**
 * Author：Alex
 * Date：2019/9/21
 * Note：
 */
class AudioGatherManager {

    companion object {
        // 音频获取
        private const val SOURCE = MediaRecorder.AudioSource.VOICE_COMMUNICATION

        // 设置音频采样率，44100是目前的标准，但是某些设备仍然支 2050 6000 1025
        private const val SAMPLE_HZ = 44100

        // 设置音频的录制的声道CHANNEL_IN_STEREO为双声道，CHANNEL_CONFIGURATION_MONO为单声道
        private const val CHANNEL_CONFIG = AudioFormat.CHANNEL_IN_STEREO

        // 音频数据格式:PCM16位，每个样本保证设备支持。PCM 8位每个样本，不一定能得到设备支持
        private const val FORMAT = AudioFormat.ENCODING_PCM_16BIT

        // 是否保存为测试文件
        private const val SAVE_FILE_FOR_TEST = false
    }

    // lateinit不能用来修饰基本类型(因为基本类型的属性在类加载后的准备阶段都会被初始化为默认值)，也不能修饰可null的变量
    private var mBufferSize: Int = 0

    private lateinit var mAudioRecord: AudioRecord
    // 回音消除器，场景就是在手机播放声音和声音录制同时进行，但是手机播放的声音不会被本机录制，达到了消除的效果
    private  var acousticEchoCanceler: AcousticEchoCanceler? = null
    // 自动增强控制器
    private  var automaticGainControl: AutomaticGainControl? = null
    private var bufferSizeInBytes = 0

    private lateinit var fileManager: FileManager

    private lateinit var workThread: Thread
    private var isLoop: Boolean = false

    init {
        // 是否保存录音
        if (SAVE_FILE_FOR_TEST) {
            fileManager = FileManager(FileManager.TEST_PCM_FILE)
        }

        bufferSizeInBytes = AudioRecord.getMinBufferSize(
            SAMPLE_HZ,
            CHANNEL_CONFIG,
            FORMAT
        )
    }

    // 录制音频数据回调方法，参数是字节数组，没有返回值，可为null
    private var audioDataListener: ((data: ByteArray) -> Unit)? = null

    fun setAudioDataListener(audioDataListener: ((data: ByteArray) -> Unit)) {
        this.audioDataListener = audioDataListener
    }

    // 初始化音频录制
    fun initAudio(): Int {
        // 音频录制对象
        mAudioRecord = AudioRecord(
            SOURCE,
            SAMPLE_HZ,
            CHANNEL_CONFIG,
            FORMAT, bufferSizeInBytes
        )

        // 回音消除
        if (AcousticEchoCanceler.isAvailable()) {
            acousticEchoCanceler = AcousticEchoCanceler.create(mAudioRecord.audioSessionId)
            acousticEchoCanceler?.setEnabled(true)
        }
        // 增强控制
        if (AutomaticGainControl.isAvailable()) {
            automaticGainControl = AutomaticGainControl.create(mAudioRecord.audioSessionId)
            automaticGainControl?.setEnabled(true)
        }
        // 初始化音频编码，返回每次编码数据的合适大小
        val bufferSize = LiveNativeManager.encoderAudioInit(LiveConfig.SAMPLE_RATE, LiveConfig.CHANNELS, LiveConfig.BIT_RATE)
        LogUtil.d("音频录制大小赋值=" + bufferSize)
        // 音频每次录制多少byte
        mBufferSize = bufferSize
        return bufferSize
    }

    private fun releaseAudio() {
        LiveNativeManager.releaseAudio()
    }

    fun startAudioGather() {

        workThread = thread(start = true) {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO);
            // 开始录制
            mAudioRecord.startRecording()
            // 保存每次读取的音频数据
            val audioData = ByteArray(mBufferSize)
            LogUtil.d("线程录制音频数据大小=" + mBufferSize)
            var readSize = 0
            isLoop = true
            // 录音，获取pcm裸音频，这个音频数据文件很大，我们必须编码成aac，主要才能 rtmp传输
            while (isLoop && !Thread.interrupted()) {
                try {
                    // 从硬件设置中读取mBufferSize音频数据到audioData中
                    readSize = mAudioRecord.read(audioData, readSize, mBufferSize)

                    if (readSize < 0) continue
                    // 复制音频pcm数据
                    val ralAudio = ByteArray(readSize)
                    System.arraycopy(audioData, 0, ralAudio, 0, readSize)
                    // 把录音的数据抛给MediaEncoder去编码成aac数据
                    audioDataListener?.let { it.invoke(ralAudio) }
                    // 我们可以把裸音频以文件格式保存起来，判断这个一片是否完好的，需要加一个WAV头
                    if (SAVE_FILE_FOR_TEST) {
                        fileManager.saveFileData(ralAudio)
                    }
                    // 重置
                    readSize = 0
                    Arrays.fill(audioData, 0)
                } catch (e: Exception) {
                    e.printStackTrace()
                }

            }

        }
    }

    fun stopAudioGather() {
        // 停止线程
        isLoop = false
        // workThread.interrupt()
        mAudioRecord.stop()
        mAudioRecord.release()
        acousticEchoCanceler?.setEnabled(false)
        acousticEchoCanceler?.release()
        automaticGainControl?.setEnabled(false)
        automaticGainControl?.release()
        // 释放jni层资源
        releaseAudio()
    }

    // 释放stream
    fun realeaseStream() {
        if (SAVE_FILE_FOR_TEST) {
            fileManager.closeFile()
        }
    }
}


























