package com.eplayer

import android.content.res.Configuration
import android.content.res.Configuration.ORIENTATION_LANDSCAPE
import android.os.Build
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.view.View
import android.view.WindowManager
import com.live.LiveConfig
import com.live.LogUtil
import com.live.MediaPublisher

import kotlinx.android.synthetic.main.activity_live.*
import kotlinx.android.synthetic.main.activity_live.camera_surface

class LiveActivity : AppCompatActivity(), View.OnClickListener {

    companion object {
        private const val rtmpUrl = "rtmp://192.168.3.19:1935/live/home"
    }

    // 音视频推流器
    private lateinit var mediaPublisher: MediaPublisher
    private var isStart = false // 是否已经开始推流
    private var mPause = true // 是否暂停推流

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_live)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION)
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        supportActionBar!!.hide()

        initView()
        mediaPublisher = MediaPublisher(this, camera_surface, rtmpUrl)
        // 初始化推流器
        mediaPublisher.init()
    }

    private fun initView() {
        // 切换摄像头
        switch_camera_img.setOnClickListener(this)
        // 开始或暂停推流
        rtmp_publish_img.setOnClickListener(this)
    }

    override fun onConfigurationChanged(newConfig: Configuration?) {
        super.onConfigurationChanged(newConfig)
        val orientation = newConfig!!.orientation
        if (orientation == ORIENTATION_LANDSCAPE) {
            LogUtil.i("-------------横屏-------------")
        } else {
            LogUtil.i("-------------竖屏-------------")
        }
        mediaPublisher.changeCarmeraOrientation()
    }

    override fun onStart() {
        super.onStart()
        // 打开摄像头
        mediaPublisher.openCamera()
    }

    override fun onResume() {
        super.onResume()
        // 如果推流中并且暂停状态
        if (isStart && mPause) {
            mPause=false
            mediaPublisher.resume()
        }
    }

    override fun onPause() {
        super.onPause()
        // 如果推流中并
        if (isStart && !mPause){
            mPause=true
            LogUtil.d("暂停")
            mediaPublisher.pause()
        }
    }

    override fun onStop() {
        super.onStop()
        // 关闭摄像头
        mediaPublisher.closeCamera()
    }

    override fun onDestroy() {
        super.onDestroy()
        // 停止推流，销毁推流上下文
        mediaPublisher.stop()
    }

    override fun onClick(v: View) {
        when (v) {
            // 切换摄像头
            switch_camera_img -> {
                mediaPublisher.switchCamera()
            }

            // 开始或暂停推流
            rtmp_publish_img -> {

                if (!isStart) { // 还没开始推流
                    isStart = true
                    mPause = false
                    rtmp_publish_img.setImageResource(R.mipmap.pause_publish)
                    mediaPublisher.start()
                } else if (mPause) { // 暂停状态
                    mPause = false
                    rtmp_publish_img.setImageResource(R.mipmap.pause_publish)
                    mediaPublisher.resume()
                } else if (!mPause) { // 推流中状态
                    mPause = true
                    rtmp_publish_img.setImageResource(R.mipmap.start_publish)
                    mediaPublisher.pause()
                }
            }
        }
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

}
