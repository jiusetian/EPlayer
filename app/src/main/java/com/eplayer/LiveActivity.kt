package com.eplayer

import android.os.Build
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.view.View
import android.view.WindowManager
import com.live.LiveConfig
import com.live.MediaPublisher
import com.live.simplertmp.RtmpHandler
import kotlinx.android.synthetic.main.activity_live.*
import java.io.IOException
import java.net.SocketException

class LiveActivity : AppCompatActivity(), View.OnClickListener, RtmpHandler.RtmpListener {

    companion object {
        private const val rtmpUrl = "rtmp://192.168.3.9:1935/live/home"
    }

    private lateinit var mediaPublisher: MediaPublisher

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_live)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION)
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        supportActionBar!!.hide()
        initView()
        mediaPublisher = MediaPublisher(this, camera_surface, rtmpUrl)
        mediaPublisher.setRtmpHandler(RtmpHandler(this))
    }

    private fun initView() {
        rtmp_url_edit_text.setText(rtmpUrl)
        switch_camera_img.setOnClickListener(this)
        rtmp_publish_img.setOnClickListener(this)
    }

    private fun initCameraInfo() {
        LiveConfig.apply {
            //摄像头预览信息
            val cameraWidth = cameraWidth
            val cameraHeight = cameraHeight
            val orientation = cameraOrientation
            tv_camera_info.setText(
                String.format(
                    "摄像头预览大小:%d*%d\n旋转的角度:%d度",
                    cameraWidth, cameraHeight, orientation
                )
            )
            if (cameraOrientation == 0 || cameraOrientation == 180) { //横屏
                //缩放大小信息
                tv_scale_width.setText(scaleWidthLand.toString())
                tv_scale_height.setText(scaleHeightLand.toString())
                //剪裁信息
                et_crop_width.setText(cropWidthLand.toString())
                et_crop_height.setText(cropHeightLand.toString())
            } else { //竖屏
                //缩放大小信息
                tv_scale_width.setText(scaleWidthVer.toString())
                tv_scale_height.setText(scaleHeightVer.toString())
                //剪裁信息
                et_crop_width.setText(cropWidthVer.toString())
                et_crop_height.setText(cropHeightVer.toString())
            }

            et_crop_start_x.setText(cropStartX.toString())
            et_crop_start_y.setText(cropStartY.toString())
        }
    }

    override fun onResume() {
        super.onResume()
        mediaPublisher.apply {
            initRtmp()
            startVideoGather()
            startAudioGather()
            startMediaEncoder()
        }
        //初始化摄像头信息
        initCameraInfo()
    }

    override fun onPause() {
        super.onPause()
    }

    override fun onStop() {
        super.onStop()
        mediaPublisher.stopVideoGather()
        mediaPublisher.stopAudioGather()
        mediaPublisher.stopMediaEncoder()
        mediaPublisher.stopPublish()
        mediaPublisher.relaseRtmp()
    }

    override fun onDestroy() {
        super.onDestroy()
        mediaPublisher.releaseIOStream()
    }

    override fun onClick(v: View) {
        when (v) {
            switch_camera_img -> {
                mediaPublisher.getVideoGatherManager().changeCamera()
                initCameraInfo()
            }

            rtmp_publish_img->{
               // mediaPublisher.getMediaEncoder().startRecord()
            }
        }
    }

    override fun onRtmpConnecting(msg: String?) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpConnected(msg: String?) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpVideoStreaming() {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpAudioStreaming() {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpStopped() {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpDisconnected() {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpVideoFpsChanged(fps: Double) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpVideoBitrateChanged(bitrate: Double) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpAudioBitrateChanged(bitrate: Double) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpSocketException(e: SocketException?) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpIOException(e: IOException?) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpIllegalArgumentException(e: IllegalArgumentException?) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun onRtmpIllegalStateException(e: IllegalStateException?) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }


}
