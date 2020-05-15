package com.live.video

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import com.live.FileManager
import com.live.LiveNativeManager
import com.live.LogUtil
import java.util.concurrent.LinkedBlockingDeque
import kotlin.concurrent.thread

/**
 * Author：Mapogo
 * Date：2020/5/15
 * Note：
 */
class VideoManager(val cameraSurface: CameraSurface, val context: Context) : VideoInterfaces, SensorEventListener {

    companion object {
        // 是否保存视频
        private const val SAVE_FILE_FOR_TEST = false
    }

    // 摄像头视频宽高
    private var videoWidth = 0
    private var videoHeight = 0

    // 视频的压缩比例
    private var scaleWidth = 0
    private var scaleHeight = 0

    // 是否已经初始化
    private var isVideoInit = false
    private var pause=false //暂停

    // 旋转角度
    private var orientation = 0

    // 传感器需要，这边使用的是加速度传感器
    private val sensorManager: SensorManager
    // 第一次实例化的时候是不需要的
    private var initialized = false
    private var lastX = 0f
    private var lastY = 0f
    private var lastZ = 0f

    private lateinit var fileManager: FileManager

    // 工作线程
    private lateinit var workerThread: Thread
    private var isLoop: Boolean = false

    // 保存摄像机采集的数据
    private val cameraDatas = LinkedBlockingDeque<ByteArray>()

    init {
        sensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
    }

    /**
     * 循环将摄像头的原始数据转化为YUV420P数据，并且进行一定程度的压缩或剪切，最后将处理好的数据传给编码模块间编码
     */
    private fun initWorkThread() {
        isLoop = true
        workerThread = thread(start = true) {
            // 循环执行
            while (isLoop && !Thread.interrupted()) {
                // 获取阻塞队列中的数据，没有数据的时候阻塞
                val srcData = cameraDatas.take()

                // 将摄像头数据交给底层编码
                LiveNativeManager.putVideoData(srcData, srcData.size)

                // 设置为true，我们把生成的YUV文件用播放器播放一下，看我们的数据是否有误，起调试作用
                if (SAVE_FILE_FOR_TEST) {
                    fileManager.saveFileData(srcData)
                }
            }
        }
    }

    override fun init() {
        // 开始线程
        initWorkThread()
        // 将视频预览数据压入队列，如果暂停的话停止添加数据
        cameraSurface.onCameraNVDataListener { it?.let { if (!pause) cameraDatas.put(it) } }

        cameraSurface.onCameraOrientationChangeListener {
            LogUtil.d("摄像头方向：" + it)
            orientation = it
        }

        cameraSurface.onCameraSizeChangeListener { width, height ->
            LogUtil.d("摄像头宽高：" + width + "---" + height)
            videoWidth = width
            videoHeight = height
            scaleWidth = videoWidth / 3
            scaleHeight = scaleWidth * 75 / 100
            // 有了摄像头视频帧的宽高，才能初始化视频编码器
            if (!isVideoInit) {
                isVideoInit = true
                LiveNativeManager.initVideoEncoder(videoWidth, videoHeight, scaleWidth, scaleHeight, cameraSurface.getCameraUtil().getCameraOrientation())
            }
        }

        if (SAVE_FILE_FOR_TEST) {
            fileManager = FileManager(FileManager.TEST_YUV_FILE)
        }
    }

    override fun start() {
        // 打开摄像头
        cameraSurface.openCamera()
        // 注册加速度传感器
        sensorManager.registerListener(
            this,
            sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
            SensorManager.SENSOR_DELAY_UI
        )
        LiveNativeManager.startVideoEncoder()
    }

    override fun stop() {
        cameraSurface.releaseCamera()
        sensorManager.unregisterListener(this)
        isLoop = false // 结束线程
        cameraDatas.clear()
        if (SAVE_FILE_FOR_TEST) {
            fileManager.closeFile()
        }
        // 底层的释放
        LiveNativeManager.stopVideoEncoder()
    }

    override fun pause() {
        pause=true
        cameraDatas.clear()
        LiveNativeManager.pauseVideoEncoder()
    }

    override fun resume() {
        pause=false
        LiveNativeManager.resumeVideoEncoder()
    }

    fun switchCamera() {
        cameraSurface.switchCamera()
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {
    }

    override fun onSensorChanged(event: SensorEvent) {
        val x = event.values[0]
        val y = event.values[1]
        val z = event.values[2]

        if (!initialized) {
            lastX = x
            lastY = y
            lastZ = z
            initialized = true
        }
        val deltaX = Math.abs(lastX - x)
        val deltaY = Math.abs(lastY - y)
        val deltaZ = Math.abs(lastZ - z)

        if (cameraSurface != null && (deltaX > 0.6 || deltaY > 0.6 || deltaZ > 0.6)) {
            cameraSurface.startAutoFocus(-1f, -1f)
        }
        lastX = x
        lastY = y
        lastZ = z
    }
}