package com.live.audio

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.media.audiofx.AcousticEchoCanceler
import android.media.audiofx.AutomaticGainControl
import android.os.Process
import com.live.common.FileManager
import com.live.common.LiveInterfaces
import com.live.LiveNativeApi
import java.util.*
import kotlin.concurrent.thread


/**
 * Author：Alex
 * Date：2019/9/21
 * Note：
 */
class AudioGatherManager : LiveInterfaces {

    companion object {
        // 音频获取
        private const val SOURCE = MediaRecorder.AudioSource.VOICE_COMMUNICATION

        // 设置音频采样率，44100是目前的标准，但是某些设备仍然支 2050 6000 1025
        private const val SAMPLE_HZ = 44100

        private const val BIT_RATE = 64000

        // 设置音频的录制的声道CHANNEL_IN_STEREO为双声道，CHANNEL_CONFIGURATION_MONO为单声道
        private const val CHANNEL_CONFIG = AudioFormat.CHANNEL_IN_STEREO

        // 音频数据格式:PCM16位，每个样本保证设备支持。PCM 8位每个样本，不一定能得到设备支持
        private const val FORMAT = AudioFormat.ENCODING_PCM_16BIT

        // 是否保存为测试文件
        private const val SAVE_FILE_FOR_TEST = false
    }

    // 一次读取音频数据的大小
    private var mBufferSize: Int = 0

    private lateinit var mAudioRecord: AudioRecord

    // 回音消除器，场景就是在手机播放声音和声音录制同时进行，但是手机播放的声音不会被本机录制，达到了消除的效果
    private var acousticEchoCanceler: AcousticEchoCanceler? = null

    // 自动增强控制器
    private var automaticGainControl: AutomaticGainControl? = null

    private var bufferSizeInBytes = 0

    private lateinit var fileManager: FileManager

    private lateinit var workThread: Thread

    private var isLoop = true

    private var mPause = true

    init {

        // 是否保存录音
        if (SAVE_FILE_FOR_TEST) {
            fileManager =
                FileManager(FileManager.TEST_PCM_FILE)
        }

        bufferSizeInBytes = AudioRecord.getMinBufferSize(
            SAMPLE_HZ,
            CHANNEL_CONFIG,
            FORMAT
        )
    }

    // 录制音频数据回调方法，参数是字节数组，没有返回值，可为null
    private var audioDataListener: ((data: ByteArray) -> Unit)? = null

    fun onAudioDataListener(audioDataListener: ((data: ByteArray) -> Unit)) {
        this.audioDataListener = audioDataListener
    }

    override fun init() {
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
        // 初始化音频编码，返回每次编码的合适大小
        val bufferSize = LiveNativeApi.initAudioEncoder(SAMPLE_HZ, 2, BIT_RATE)

        // 每次录制多少byte
        mBufferSize = bufferSize
    }

    override fun start() {
        mPause = false
        // 录音线程
        workThread = thread(start = true) {
            Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);
            // 开始录制
            mAudioRecord.startRecording()
            // 保存一次读取的音频数据
            val audioData = ByteArray(mBufferSize)
            var readSize = 0

            // 录音，获取pcm裸音频，这个音频数据文件很大，必须编码成aac，才能 rtmp传输
            while (isLoop && !Thread.interrupted()) {
                try {
                    // 暂停的话不读音频
                    if (mPause) {
                        continue
                    }
                    // 从硬件设置中读取 mBufferSize音频数据到audioData中
                    readSize = mAudioRecord.read(audioData, readSize, mBufferSize)

                    if (readSize < 0) continue

                    // 复制音频pcm数据
                    val audioCopy = ByteArray(readSize)
                    System.arraycopy(audioData, 0, audioCopy, 0, readSize)
                    // 把录音的数据抛给MediaEncoder去编码成aac数据
                    audioDataListener?.let { it.invoke(audioCopy) }
                    // 可以把裸音频以文件格式保存起来，进行测试
                    if (SAVE_FILE_FOR_TEST) {
                        fileManager.saveFileData(audioCopy)
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

    override fun stop() {
        // 停止线程
        isLoop = false

        mAudioRecord.stop()
        mAudioRecord.release()
        acousticEchoCanceler?.setEnabled(false)
        acousticEchoCanceler?.release()
        automaticGainControl?.setEnabled(false)
        automaticGainControl?.release()
        // 释放文件流
        if (SAVE_FILE_FOR_TEST) {
            fileManager.closeFile()
        }
        // 释放底层资源
        LiveNativeApi.releaseAudio()
    }

    override fun destrory() {
        TODO("Not yet implemented")
    }

    override fun pause() {
        mPause = true
    }

    override fun resume() {
        mPause = false
    }
}


























