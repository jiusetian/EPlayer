package com.eplayer

import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.os.Build
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.text.TextUtils
import android.util.Log
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.View
import android.view.WindowManager
import android.widget.SeekBar
import kotlinx.android.synthetic.main.activity_media_player.*


class MediaPlayerActivity : AppCompatActivity(), View.OnClickListener, SeekBar.OnSeekBarChangeListener,
    SurfaceHolder.Callback {

    companion object {
        private const val PATH = "path"
    }

    private lateinit var path: String
    private lateinit var eMediaPlayer: EasyMediaPlayer
    private var mProgress = 0
    private var screenOrientation: Int = Configuration.ORIENTATION_UNDEFINED
    private var isPlayComplete = false //播放是否完成



    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_media_player)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION)
        }
        supportActionBar!!.hide()
        path = intent.getStringExtra(PATH)
        initView()
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
            //开始播放
            eMediaPlayer.start()
            //UI线程执行
            runOnUiThread {
                //改变Surfaceview的宽高
                changeSurfaceSize()
                //播放进度相关
                tv_current_position.setText(
                    StringUtils.generateStandardTime(Math.max(eMediaPlayer.getCurrentPosition(), 0))
                )
                tv_duration.setText(StringUtils.generateStandardTime(Math.max(eMediaPlayer.getDuration(), 0)))
                seekbar.setMax(Math.max(eMediaPlayer.getDuration(), 0).toInt())
                seekbar.setProgress(Math.max(eMediaPlayer.getCurrentPosition(), 0).toInt())
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
            iv_pause_play.setImageResource(R.mipmap.iv_replay)
        }

        //播放进度监听
        eMediaPlayer.setOnCurrentPositionListener { current, duration ->
            runOnUiThread {
                if (eMediaPlayer.isPlaying) {
                    //设置当前播放时间
                    tv_current_position.setText(StringUtils.generateStandardTime(current))
                    //播放进度
                    seekbar.setProgress(current.toInt())
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

    //初始化view
    private fun initView() {
        screenOrientation = resources.configuration.orientation
        iv_pause_play.setOnClickListener(this)
        seekbar.setOnSeekBarChangeListener(this)
        //横竖屏切换
        iv_orientation.setImageResource(if (screenOrientation == Configuration.ORIENTATION_PORTRAIT) R.mipmap.iv_land else R.mipmap.iv_port)
        iv_orientation.setOnClickListener(this)

        surfaceView.holder.addCallback(this)
        surfaceView.setOnTouchListener { v, event ->
            if (event.action == MotionEvent.ACTION_DOWN) {
                layout_operation.visibility =
                    if (layout_operation.visibility == View.VISIBLE) View.GONE else View.VISIBLE
            }
            true
        }
    }

    override fun onPause() {
        super.onPause()
        eMediaPlayer.pause()
    }

    override fun onResume() {
        super.onResume()
        eMediaPlayer.resume()
    }

    override fun onDestroy() {
        super.onDestroy()
        eMediaPlayer.stop()
        eMediaPlayer.release()
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        //屏幕方向变化
        if (newConfig.orientation != screenOrientation) {
            screenOrientation = newConfig.orientation
            iv_orientation.setImageResource(if (screenOrientation == Configuration.ORIENTATION_PORTRAIT) R.mipmap.iv_land else R.mipmap.iv_port)
            changeSurfaceSize()
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
        val layoutParams = surfaceView.getLayoutParams()
        layoutParams.width = viewWidth
        layoutParams.height = viewHeight
        surfaceView.setLayoutParams(layoutParams)
        surfaceView.requestLayout()

    }

    //根据视频宽高适配Surfaceview的宽高
    private fun fitSurfaceSize(videoWidth:Int,videoHeight:Int){
        val vRatio=videoWidth/videoHeight
        val sRatio=Utils.getScreenWidth(this)/Utils.getScreenHeight(this)
        //最终view的宽高
        var viewWidth:Int
        var viewHeight:Int

        if (vRatio>sRatio){
            viewWidth=Utils.getScreenWidth(this)
            viewHeight=Utils.getScreenHeight(this)*(sRatio/vRatio)
        }else if (vRatio<sRatio){

        }
    }


    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {

    }

    override fun surfaceDestroyed(holder: SurfaceHolder?) {}

    override fun surfaceCreated(holder: SurfaceHolder?) {
        //设置播放器的渲染界面
        eMediaPlayer.setDisplay(surfaceView.holder)
    }

    override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
        if (fromUser) {
            mProgress = progress
            seekBar.setProgress(progress)
            //设置当前播放时间
            tv_current_position.setText(StringUtils.generateStandardTime(mProgress.toLong()))
        }
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {

    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
        eMediaPlayer.seekTo(mProgress.toFloat())
    }

    override fun onClick(v: View) {
        //播放暂停
        if (v.id == R.id.iv_pause_play) {
            if (eMediaPlayer.isPlaying && !isPlayComplete) { //暂停
                LogUtil.d("边距="+surfaceView.paddingTop)
                eMediaPlayer.pause()
                iv_pause_play.setImageResource(R.drawable.ic_player_play)
            } else if (!isPlayComplete) { //播放
                eMediaPlayer.resume()
                iv_pause_play.setImageResource(R.drawable.ic_player_pause)
            } else if (isPlayComplete) { //重播
                eMediaPlayer.seekTo(0f)
                isPlayComplete = false
                iv_pause_play.setImageResource(R.drawable.ic_player_pause)
            }
        }

        //横竖屏切换
        if (v.id == R.id.iv_orientation) {
            requestedOrientation =
                if (screenOrientation == Configuration.ORIENTATION_PORTRAIT) ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE
                else ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT
        }
    }
}
