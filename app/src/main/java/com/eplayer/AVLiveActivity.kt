package com.eplayer

import android.content.res.Configuration
import android.os.Build
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.view.View
import android.view.WindowManager
import com.live.AVPublisher
import com.live.LiveNativeManager
import com.live.LogUtil
import kotlinx.android.synthetic.main.activity_a_v_live.*

class AVLiveActivity : AppCompatActivity(), View.OnClickListener {

    companion object {
        private const val rtmpUrl = "rtmp://192.168.3.19:1935/live/home"
    }

    // 音视频推流器
    private lateinit var avPublisher: AVPublisher

    private var isPublish = false // 是否正在推流

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_a_v_live)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION)
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        supportActionBar!!.hide()

        initView()
        avPublisher = AVPublisher(this, camera_surface, rtmpUrl)
        avPublisher.init() // 初始化推流器
    }

    private fun initView() {
        switch_camera.setOnClickListener(this)
        rtmp_publish.setOnClickListener(this)
    }

    override fun onStart() {
        super.onStart()
        avPublisher.openCamera()
    }

    override fun onResume() {
        super.onResume()
    }

    override fun onStop() {
        super.onStop()
        avPublisher.stop()
        avPublisher.release() // 释放资源
    }

    override fun onConfigurationChanged(newConfig: Configuration?) {
        super.onConfigurationChanged(newConfig)
        val orientation = newConfig!!.orientation
        if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
            LogUtil.i("-------------横屏-------------")
        } else {
            LogUtil.i("-------------竖屏-------------")
        }
        avPublisher.changeCarmeraOrientation()
    }

    override fun onClick(v: View?) {
        when (v) {
            // 切换相机
            switch_camera -> {
                avPublisher.switchCamera()
            }

            // 推流
            rtmp_publish -> {
                if (!isPublish) {
                    isPublish = true
                    rtmp_publish.setImageResource(R.mipmap.pause_publish)
                    avPublisher.start()
                } else {
                    isPublish = false
                    rtmp_publish.setImageResource(R.mipmap.start_publish)
                    avPublisher.pause()
                }

            }
        }
    }
}
