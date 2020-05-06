package com.eplayer

import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.graphics.*
import android.opengl.Matrix
import android.os.Build
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.text.TextUtils
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.View
import android.view.WindowManager
import android.widget.SeekBar
import com.eplayer.common.LogUtil
import com.eplayer.common.StringUtils
import com.eplayer.common.Utils
import kotlinx.android.synthetic.main.activity_media_player.*
import java.nio.ByteBuffer


class MediaPlayerActivity : AppCompatActivity(), View.OnClickListener, SeekBar.OnSeekBarChangeListener,
    SurfaceHolder.Callback {

    companion object {
        private const val PATH = "path"
    }

    //private lateinit var path: String
    private lateinit var paths: List<String>
    private var index = 0
    private lateinit var mediaPlayer: EasyMediaPlayer
    private var mProgress = 0
    private var screenOrientation: Int = Configuration.ORIENTATION_UNDEFINED
    private var isPlayComplete = false // 播放是否完成
    private var isTwoScreen = false // 是否双屏播放


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_media_player)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            window.addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION)
        }
        supportActionBar!!.hide()
        paths = Utils.getVideoList(this)
        index = paths.size - 2
        initPlayer(paths.get(index))
        initView()
    }


    // 初始化播放器
    private fun initPlayer(path: String) {
        if (TextUtils.isEmpty(path)) return

        mediaPlayer = EasyMediaPlayer()
        // 设置播放源路径
        try {
            mediaPlayer.setDataSource(path)
        } catch (e: Exception) {
            e.printStackTrace()
        }
        // 监听尺寸的改变
        mediaPlayer.setOnVideoSizeChangedListener { mediaPlayer, width, height ->
            LogUtil.d("视频size改变=" + width + "--" + height)
        }

        // 设置准备监听
        mediaPlayer.setOnPreparedListener {
            LogUtil.d("开始播放")
            // 开始播放
            mediaPlayer.start()
            // UI线程执行
            runOnUiThread {
                // 播放进度相关
                tv_current_position.setText(
                    StringUtils.generateStandardTime(Math.max(mediaPlayer.getCurrentPosition(), 0))
                )
                tv_duration.setText(StringUtils.generateStandardTime(Math.max(mediaPlayer.getDuration(), 0)))
                seekbar.setMax(Math.max(mediaPlayer.getDuration(), 0).toInt())
                seekbar.setProgress(Math.max(mediaPlayer.getCurrentPosition(), 0).toInt())
            }
        }

        // 设置错误监听
        mediaPlayer.setOnErrorListener { mp, what, extra ->
            LogUtil.d("tag", "发生错误：$what,$extra")
            false
        }

        // 完成监听
        mediaPlayer.setOnCompletionListener {
            Thread.sleep(1000)
            // 已经播放完了
            isPlayComplete = true
            iv_pause_play.setImageResource(R.mipmap.iv_replay)
        }

        // 播放进度监听
        mediaPlayer.setOnCurrentPositionListener { current, duration ->
            runOnUiThread {
                if (mediaPlayer.isPlaying) {
                    // 设置当前播放时间
                    tv_current_position.setText(StringUtils.generateStandardTime(current))
                    // 播放进度
                    seekbar.setProgress(current.toInt())
                }
            }
        }

        // 准备播放
        try {
            // 设置播放器参数，指定视频解码器名称为h264_mediacodec
            mediaPlayer.setOption(EasyMediaPlayer.OPT_CATEGORY_PLAYER, "vcodec", "h264_mediacodec")
            mediaPlayer.prepareAsync()

        } catch (e: Exception) {
            e.printStackTrace()
        }

    }

    // 初始化view
    private fun initView() {
        // 初始化点击事件
        initClicker()
        screenOrientation = resources.configuration.orientation
        seekbar.setOnSeekBarChangeListener(this)
        // 横竖屏切换图标
        iv_orientation.setImageResource(if (screenOrientation == Configuration.ORIENTATION_PORTRAIT) R.mipmap.iv_land else R.mipmap.iv_port)
        surfaceView.holder.addCallback(this)
        surfaceView.setOnTouchListener { v, event ->
            if (event.action == MotionEvent.ACTION_DOWN) {
                layout_operation.visibility =
                    if (layout_operation.visibility == View.VISIBLE) View.GONE else View.VISIBLE
            }
            true
        }
    }

    private fun initClicker() {
        arrayOf(
            iv_pause_play,
            iv_orientation,
            action_original,
            action_blackwhite,
            action_dim,
            action_cooltone,
            action_warmtone,
            action_twoscreen,
            action_next,
            action_prev,
            action_soul,
            action_shake,
            action_clitterwihte,
            action_illusion,
            action_scale,
            action_blursplit,
            action_blackwhitethree,
            action_two,
            action_three,
            action_four,
            action_six,
            action_nine,
            action_watermark
        ).forEach { it.setOnClickListener(this) }
    }

    override fun onPause() {
        super.onPause()
        mediaPlayer.pause()
    }

    override fun onResume() {
        super.onResume()
        mediaPlayer.resume()
    }

    override fun onDestroy() {
        super.onDestroy()
        mediaPlayer.stop()
        mediaPlayer.release()
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        // 屏幕方向变化
        if (newConfig.orientation != screenOrientation) {
            screenOrientation = newConfig.orientation
            iv_orientation.setImageResource(if (screenOrientation == Configuration.ORIENTATION_PORTRAIT) R.mipmap.iv_land else R.mipmap.iv_port)
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {
        LogUtil.d("Surface的宽高=" + width + "----" + height)
        mediaPlayer.surfaceChange(width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder?) {}

    override fun surfaceCreated(holder: SurfaceHolder?) {
        // 设置播放器的渲染界面,这个surfaceview会传递到NDK底层
        mediaPlayer.setDisplay(surfaceView.holder)
    }

    override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
        if (fromUser) { // 是否为用户行为，比如用户拖动seekbar
            mProgress = progress
            seekBar.setProgress(progress)
            // 设置当前播放时间
            tv_current_position.setText(StringUtils.generateStandardTime(mProgress.toLong()))
        }
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {}

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
        // 改变视频播放进度
        mediaPlayer.seekTo(mProgress.toFloat())
        if (isPlayComplete) {
            iv_pause_play.setImageResource(R.drawable.ic_player_pause)
            isPlayComplete = false
        }
    }

    // 播放
    private fun play(path: String) {
        LogUtil.d("播放路径=" + path)
        mediaPlayer.reset()
        initPlayer(path)
        mediaPlayer.setDisplay(surfaceView.holder)
    }

    // 添加水印
    private fun addImageWatermark() {
        val options = BitmapFactory.Options()
        options.inPreferredConfig = Bitmap.Config.ARGB_8888
        val bitmap = BitmapFactory.decodeResource(resources, R.mipmap.ic_launcher, options)
        val byteCount = bitmap.byteCount
        val buffer = ByteBuffer.allocate(byteCount)
        bitmap.copyPixelsToBuffer(buffer)
        buffer.position(0)

        val mWatermark = buffer.array()
        val mWatermarkWidth = bitmap.width
        val mWatermarkHeight = bitmap.height
        bitmap.recycle()

        mediaPlayer.setWatermark(mWatermark, mWatermark.size, mWatermarkWidth, mWatermarkHeight, 7f, 2)
    }

    // 添加文字水印
    fun addTextWatermark(text: String, width: Int, height: Int) {
        val textBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(textBitmap)
        val paint = Paint()
        paint.color = Color.argb(255, 255, 0, 0)
        paint.textSize = 28f
        paint.isAntiAlias = true
        paint.textAlign = Paint.Align.CENTER
        val rect = Rect(0, 0, width, height)
        val fontMetrics = paint.fontMetricsInt
        // 将文字绘制在矩形区域的正中间
        val baseline = (rect.bottom + rect.top - fontMetrics.bottom - fontMetrics.top) / 2
        canvas.drawText(text, rect.centerX().toFloat(), baseline.toFloat(), paint)
        val capacity = width * height * 4
        val buffer = ByteBuffer.allocate(capacity)
        textBitmap.copyPixelsToBuffer(buffer)
        buffer.position(0)

        val mWatermark = buffer.array()
        val mWatermarkWidth = textBitmap.width
        val mWatermarkHeight = textBitmap.height
        textBitmap.recycle()

        mediaPlayer.setWatermark(mWatermark, mWatermark.size, mWatermarkWidth, mWatermarkHeight, 5f, 2)
    }

    override fun onClick(v: View) {
        when (v.id) {
            // 播放暂停
            R.id.iv_pause_play -> {
                if (mediaPlayer.isPlaying && !isPlayComplete) { // 暂停
                    mediaPlayer.pause()
                    iv_pause_play.setImageResource(R.drawable.ic_player_play)
                } else if (!isPlayComplete) { // 播放
                    mediaPlayer.resume()
                    iv_pause_play.setImageResource(R.drawable.ic_player_pause)
                } else if (isPlayComplete) { // 重播
                    mediaPlayer.seekTo(0f)
                    isPlayComplete = false
                    iv_pause_play.setImageResource(R.drawable.ic_player_pause)
                }
            }
            // 横竖屏切换
            R.id.iv_orientation -> requestedOrientation =
                if (screenOrientation == Configuration.ORIENTATION_PORTRAIT) ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE
                else ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT

            // 原色
            R.id.action_original -> mediaPlayer.changeBackground(Background.NONE.bgName)
            // 黑白
            R.id.action_blackwhite -> mediaPlayer.changeBackground(Background.GRAY.bgName)
            // 冷色调
            R.id.action_cooltone -> mediaPlayer.changeBackground(Background.COOL.bgName)
            // 模糊
            R.id.action_dim -> mediaPlayer.changeBackground(Background.BLUR.bgName)
            // 暖色调
            R.id.action_warmtone -> mediaPlayer.changeBackground(Background.WARM.bgName)

            // 双屏播放
            R.id.action_twoscreen -> {
            }

            // 播放上一个
            R.id.action_prev -> {
                index = if (index == 0) paths.size - 1 else --index
                play(paths.get(index))
                isPlayComplete = false
                iv_pause_play.setImageResource(R.drawable.ic_player_pause)
            }


            // 播放下一个
            R.id.action_next -> {
                index = if (index == paths.size - 1) 0 else ++index
                play(paths.get(index))
                isPlayComplete = false
                iv_pause_play.setImageResource(R.drawable.ic_player_pause)
            }

            // 灵魂出窍
            R.id.action_soul -> mediaPlayer.changeFilter(Filter.SOUL.filterName)
            // 抖动
            R.id.action_shake -> mediaPlayer.changeFilter(Filter.SHAKE.filterName)
            // 幻觉
            R.id.action_illusion -> mediaPlayer.changeFilter(Filter.ILLUSION.filterName)
            // 缩放
            R.id.action_scale -> mediaPlayer.changeFilter(Filter.SCALE.filterName)
            // 闪白
            R.id.action_clitterwihte -> mediaPlayer.changeFilter(Filter.GLITTERWHITE.filterName)

            // 模糊分屏
            R.id.action_blursplit -> mediaPlayer.changeEffect(Effect.BLURSPLIT.effectName)
            // 黑白三屏
            R.id.action_blackwhitethree -> mediaPlayer.changeEffect(Effect.BLACKWHITETHREE.effectName)
            // 两屏
            R.id.action_two -> mediaPlayer.changeEffect(Effect.TWO.effectName)
            // 三屏
            R.id.action_three -> mediaPlayer.changeEffect(Effect.THREE.effectName)
            // 四屏
            R.id.action_four -> mediaPlayer.changeEffect(Effect.FOUR.effectName)
            // 六屏
            R.id.action_six -> mediaPlayer.changeEffect(Effect.SIX.effectName)
            // 九屏
            R.id.action_nine -> mediaPlayer.changeEffect(Effect.NINE.effectName)

            // 水印
            R.id.action_watermark -> {
                //addImageWatermark()
                addTextWatermark("刘兴荣",100,100)
            }

        }
    }

    // 背景颜色
    enum class Background(val bgName: String) {
        NONE("原色"),
        GRAY("黑白"),
        COOL("冷色调"),
        WARM("暖色调"),
        BLUR("模糊");
    }

    // 滤镜类型
    enum class Filter(val filterName: String) {
        SOUL("灵魂出窍"),
        SHAKE("抖动"),
        ILLUSION("幻觉"),
        SCALE("缩放"),
        GLITTERWHITE("闪白"),
    }

    // 特效类型
    enum class Effect(val effectName: String) {
        BLURSPLIT("模糊分屏"),
        BLACKWHITETHREE("黑白三屏"),
        TWO("两屏"),
        THREE("三屏"),
        FOUR("四屏"),
        SIX("六屏"),
        NINE("九屏"),
    }

}
