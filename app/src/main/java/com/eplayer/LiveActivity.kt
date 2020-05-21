package com.eplayer

import android.content.res.Configuration
import android.content.res.Configuration.ORIENTATION_LANDSCAPE
import android.os.Build
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.view.View
import android.view.WindowManager
import com.live.common.LogUtil
import com.live.avlive1.MediaPusher

import kotlinx.android.synthetic.main.activity_live.*
import kotlinx.android.synthetic.main.activity_live.camera_surface

class LiveActivity : AppCompatActivity(), View.OnClickListener {

    companion object {
        private const val rtmpUrl = "rtmp://192.168.3.19:1935/live/home"
    }

    // 音视频推流器
    private lateinit var mediaPusher: MediaPusher
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
        mediaPusher = MediaPusher(this, camera_surface, rtmpUrl)
        // 初始化推流器
        mediaPusher.init()
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
            LogUtil.i("横屏")
        } else {
            LogUtil.i("竖屏")
        }
        mediaPusher.changeCarmeraOrientation()
    }

    override fun onStart() {
        super.onStart()
        // 打开摄像头
        mediaPusher.openCamera()
    }

    override fun onResume() {
        super.onResume()
        // 如果推流中并且暂停状态
        if (isStart && mPause) {
            mPause=false
            mediaPusher.resume()
        }
    }

    override fun onPause() {
        super.onPause()
        // 如果推流中并
        if (isStart && !mPause){
            mPause=true
            LogUtil.d("暂停")
            mediaPusher.pause()
        }
    }

    override fun onStop() {
        super.onStop()
        // 关闭摄像头
        mediaPusher.closeCamera()
    }

    override fun onDestroy() {
        super.onDestroy()
        // 停止推流，销毁推流上下文
        mediaPusher.stop()
    }

    override fun onClick(v: View) {
        when (v) {
            // 切换摄像头
            switch_camera_img -> {
                mediaPusher.switchCamera()
            }

            // 开始或暂停推流
            rtmp_publish_img -> {

                if (!isStart) { // 开始推流
                    isStart = true
                    mPause = false
                    rtmp_publish_img.setImageResource(R.mipmap.pause_publish)
                    mediaPusher.start()
                } else if (mPause) { // 开始
                    mPause = false
                    rtmp_publish_img.setImageResource(R.mipmap.pause_publish)
                    mediaPusher.resume()
                } else if (!mPause) { // 暂停
                    mPause = true
                    rtmp_publish_img.setImageResource(R.mipmap.start_publish)
                    mediaPusher.pause()
                }
            }
        }
    }

}
