package com.live.avlive2

import android.content.Context
import com.live.LiveNativeApi
import com.live.common.PublishInterfaces
import com.live.video.CameraSurface
import kotlin.concurrent.thread

/**
 * Author：Mapogo
 * Date：2020/5/16
 * Note：
 */
class AVPublisher(val context: Context, val cameraSurface: CameraSurface, val rtmpUrl: String) :
    PublishInterfaces {


    private lateinit var videoManager: VideoManager

    private lateinit var audioManager: AudioManager


    override fun init() {
        videoManager = VideoManager(cameraSurface, context)
        audioManager = AudioManager()
        videoManager.init()
        audioManager.init()

        thread(start = true) {
            LiveNativeApi.initRtmpData(rtmpUrl)
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
        LiveNativeApi.startRtmpPublish()
    }

    override fun stop() {
        videoManager.stop()
        audioManager.stop()
        LiveNativeApi.stopRtmpPublish()
    }

    override fun pause() {
        videoManager.pause()
        audioManager.pause()
        LiveNativeApi.pauseRtmpPublish()
    }

    override fun resume() {
        videoManager.resume()
        audioManager.resume()
        LiveNativeApi.resumeRtmpPublish()
    }

    override fun release() {
        LiveNativeApi.release()
    }


}