package com.eplayer

import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.os.Build
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.text.TextUtils
import android.util.Log
import android.view.*
import android.widget.SeekBar
import com.eplayer.common.LogUtil
import com.eplayer.common.StringUtils
import com.eplayer.common.Utils
import com.live.LiveConfig
import com.live.MediaPublisher
import com.live.simplertmp.RtmpHandler
import kotlinx.android.synthetic.main.activity_live.*
import kotlinx.android.synthetic.main.activity_two_screen.*
import java.io.IOException
import java.net.SocketException

/**
 * 推流和播放器的分屏
 */
class TwoScreenActivity : AppCompatActivity(), View.OnClickListener, RtmpHandler.RtmpListener
    , SeekBar.OnSeekBarChangeListener, SurfaceHolder.Callback {

    companion object {
        private const val rtmpUrl = "rtmp://192.168.3.9:1935/live/home"
        private const val PATH = "path"
    }

    private lateinit var mediaPublisher: MediaPublisher

    private lateinit var path: String
    private lateinit var eMediaPlayer: EasyMediaPlayer
    private var mProgress = 0
    private var screenOrientation: Int = Configuration.ORIENTATION_UNDEFINED
    private var isPlayComplete = false //播放是否完成

    private lateinit var surfaceView0: SurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_two_screen)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION)
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        supportActionBar!!.hide()
        initView()
        mediaPublisher = MediaPublisher(this, camera_surface0, rtmpUrl)
        mediaPublisher.setRtmpHandler(RtmpHandler(this))

        supportActionBar!!.hide()
        path = intent.getStringExtra(PATH)
        initPlayer()
    }

    //初始化播放器
    private fun initPlayer() {

        if (TextUtils.isEmpty(path)) return

        eMediaPlayer = EasyMediaPlayer()
        //设置播放源路径
        try {
            eMediaPlayer.setDataSource(path)
        } catch (e: Exception) {
            e.printStackTrace()
        }
        //监听适配尺寸的改变
        eMediaPlayer.setOnVideoSizeChangedListener { mediaPlayer, width, height ->
            LogUtil.d("视频size改变=" + width + "--" + height)
        }

        //设置准备监听
        eMediaPlayer.setOnPreparedListener {
            LogUtil.d("播放器已准备")
            //开始播放
            eMediaPlayer.start()
            //UI线程执行
            runOnUiThread {
                //改变Surfaceview的宽高
                changeSurfaceSize()
                //播放进度相关
                tv_current_position0.setText(
                    StringUtils.generateStandardTime(
                        Math.max(
                            eMediaPlayer.getCurrentPosition(),
                            0
                        )
                    )
                )
                tv_duration0.setText(StringUtils.generateStandardTime(Math.max(eMediaPlayer.getDuration(), 0)))
                seekbar0.setMax(Math.max(eMediaPlayer.getDuration(), 0).toInt())
                seekbar0.setProgress(Math.max(eMediaPlayer.getCurrentPosition(), 0).toInt())
            }

        }

        //设置错误监听
        eMediaPlayer.setOnErrorListener { mp, what, extra ->
            Log.d("tag", "发生错误：$what,$extra")
            false
        }

        //完成监听
        eMediaPlayer.setOnCompletionListener {
            isPlayComplete = true
            iv_pause_play0.setImageResource(R.mipmap.iv_replay)
        }

        //播放进度监听
        eMediaPlayer.setOnCurrentPositionListener { current, duration ->
            runOnUiThread {
                if (eMediaPlayer.isPlaying) {
                    //设置当前播放时间
                    tv_current_position0.setText(StringUtils.generateStandardTime(current))
                    //播放进度
                    seekbar0.setProgress(current.toInt())
                }
            }
        }

        //准备播放
        try {
            //设置播放器参数，指定视频解码器名称为h264_mediacodec
            eMediaPlayer.setOption(EasyMediaPlayer.OPT_CATEGORY_PLAYER, "vcodec", "h264_mediacodec")
            eMediaPlayer.prepareAsync()

        } catch (e: Exception) {
            e.printStackTrace()
        }

    }

    //横竖屏切换，改变Surfaceview的大小
    private fun changeSurfaceSize() {
        //默认Surfaceview的宽等于屏幕的宽，高按视频帧的宽高比适配
        val viewWidth = Utils.getScreenWidth(this)
        var viewHeight: Int

        if (eMediaPlayer.getRotate() % 180 !== 0) {
            viewHeight =
                if (eMediaPlayer.videoHeight == 0) viewWidth / 2 else viewWidth * eMediaPlayer.getVideoWidth() / eMediaPlayer.getVideoHeight()
        } else {
            viewHeight =
                if (eMediaPlayer.videoWidth == 0) viewWidth / 2 else viewWidth * eMediaPlayer.getVideoHeight() / eMediaPlayer.getVideoWidth()
        }
        LogUtil.d("视频的宽高=" + eMediaPlayer.videoWidth + "--" + eMediaPlayer.videoHeight)
        LogUtil.d("surface的宽高=" + viewWidth + "--" + viewHeight)
        LogUtil.d("屏幕的宽高=" + Utils.getScreenWidth(this) + "--" + Utils.getScreenHeight(this))
        val layoutParams = surfaceView0.getLayoutParams()
        layoutParams.width = viewWidth
        layoutParams.height = viewHeight
        surfaceView0.setLayoutParams(layoutParams)
        surfaceView0.requestLayout()

    }

    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {}

    override fun surfaceDestroyed(holder: SurfaceHolder?) {}

    override fun surfaceCreated(holder: SurfaceHolder?) {
        LogUtil.d("渲染界面="+surfaceView0)
        //设置播放器的渲染界面
        eMediaPlayer.setDisplay(surfaceView0.holder)
    }

    override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
        if (fromUser) {
            mProgress = progress
            seekbar0.setProgress(progress)
            //设置当前播放时间
            tv_current_position0.setText(StringUtils.generateStandardTime(mProgress.toLong()))
        }
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {

    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
        eMediaPlayer.seekTo(mProgress.toFloat())
    }

    private fun initView() {
        surfaceView0=findViewById(R.id.surfaceView0)
        // rtmp_url_edit_text.setText(rtmpUrl)
        switch_camera_img0.setOnClickListener(this)
//        rtmp_publish_img.setOnClickListener(this)

        screenOrientation = resources.configuration.orientation
        iv_pause_play0.setOnClickListener(this)
        seekbar0.setOnSeekBarChangeListener(this)
        //横竖屏切换
        iv_orientation0.setImageResource(if (screenOrientation == Configuration.ORIENTATION_PORTRAIT) R.mipmap.iv_land else R.mipmap.iv_port)
        iv_orientation0.setOnClickListener(this)

        surfaceView0.holder.addCallback(this)
        surfaceView0.setOnTouchListener { v, event ->
            if (event.action == MotionEvent.ACTION_DOWN) {
                layout_operation0.visibility =
                    if (layout_operation0.visibility == View.VISIBLE) View.GONE else View.VISIBLE
            }
            true
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

    override fun onResume() {
        super.onResume()
        mediaPublisher.apply {
            initRtmp()
            startVideoGather()
            startAudioGather()
            startMediaEncoder()
        }

        eMediaPlayer.resume()
        //初始化摄像头信息
        //initCameraInfo()
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

        eMediaPlayer.stop()
        eMediaPlayer.release()
    }

    override fun onClick(v: View) {

        //播放暂停
        if (v.id == R.id.iv_pause_play0) {
            if (eMediaPlayer.isPlaying && !isPlayComplete) { //暂停
                eMediaPlayer.pause()
                LogUtil.d("边距="+surfaceView0.paddingTop)
                iv_pause_play0.setImageResource(R.drawable.ic_player_play)
            } else if (!isPlayComplete) { //播放
                eMediaPlayer.resume()
                iv_pause_play0.setImageResource(R.drawable.ic_player_pause)
            } else if (isPlayComplete) { //重播
                eMediaPlayer.seekTo(0f)
                isPlayComplete = false
                iv_pause_play0.setImageResource(R.drawable.ic_player_pause)
            }
        }

        //横竖屏切换
        if (v.id == R.id.iv_orientation0) {
            requestedOrientation =
                if (screenOrientation == Configuration.ORIENTATION_PORTRAIT) ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE
                else ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT
        }

        when (v) {
            switch_camera_img0 -> {
                mediaPublisher.getVideoGatherManager().changeCamera()
                //initCameraInfo()
            }

            rtmp_publish_img -> {
                // mediaPublisher.getMediaEncoder().startRecord()
            }
        }
    }

    override fun onPause() {
        super.onPause()
        eMediaPlayer.pause()
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
