package com.live

import android.content.Context
import com.live.audio.AudioManager
import com.live.video.CameraSurface
import com.live.video.VideoManager
import kotlin.concurrent.thread

/**
 * Author：Mapogo
 * Date：2020/5/16
 * Note：
 */
class AVPublisher(val context: Context, val cameraSurface: CameraSurface, val rtmpUrl: String) : PublishInterfaces {


    private lateinit var videoManager: VideoManager

    private lateinit var audioManager: AudioManager


    override fun init() {
        videoManager = VideoManager(cameraSurface, context)
        audioManager = AudioManager()
        videoManager.init()
        audioManager.init()

        thread(start = true) {
            LiveNativeManager.initRtmpData(rtmpUrl)
        }
    }

    fun switchCamera(){
        videoManager.switchCamera()
    }

    fun openCamera(){
        videoManager.openCamera()
    }

    // 改变摄像机方向
    fun changeCarmeraOrientation(): Int {
        return videoManager.changeCarmeraOrientation()
    }


    override fun start() {
        videoManager.start()
        audioManager.start()
        LiveNativeManager.startRtmpPublish()
    }

    override fun stop() {
        videoManager.stop()
        audioManager.stop()
        LiveNativeManager.stopRtmpPublish()
    }

    override fun pause() {
        videoManager.pause()
        audioManager.pause()
        LiveNativeManager.pauseRtmpPublish()
    }

    override fun resume() {
        videoManager.resume()
        audioManager.resume()
        LiveNativeManager.resumeRtmpPublish()
    }

    override fun release() {
        LiveNativeManager.release()
    }


}