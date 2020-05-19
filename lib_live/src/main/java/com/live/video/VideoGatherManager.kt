package com.live.video

import android.content.Context
import android.content.Context.SENSOR_SERVICE
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import com.live.FileManager
import com.live.LiveInterfaces
import com.live.LiveNativeManager
import com.live.LogUtil
import java.util.concurrent.LinkedBlockingDeque
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.concurrent.thread

/**
 * Author：Alex
 * Date：2019/10/4
 * Note：
 */
class VideoGatherManager(val cameraSurface: CameraSurface, val context: Context) : SensorEventListener,
    LiveInterfaces {

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

    // 旋转角度
    private var orientation = 0

    // 视频编码器是否已初始化
    private var isVideoInit = false

    // 是否暂停
    private var mPause: AtomicBoolean

    // 第一次实例化的时候是不需要的
    private var initialized = false
    private var lastX = 0f
    private var lastY = 0f
    private var lastZ = 0f

    // 加速度传感器
    private val sensorManager: SensorManager

    // 相机数据
    private val cameraDatas = LinkedBlockingDeque<ByteArray>()

    // 压缩数据的线程
    private lateinit var compressThread: Thread
    private lateinit var fileManager: FileManager

    // 线程是否工作
    private var isWork: Boolean = true

    // 压缩回调接口
    private lateinit var compressDataListener: ((data: ByteArray, width: Int, height: Int) -> Unit)

    init {
        mPause=AtomicBoolean(true)
        sensorManager = context.getSystemService(SENSOR_SERVICE) as SensorManager
        if (SAVE_FILE_FOR_TEST) {
            fileManager = FileManager(FileManager.TEST_YUV_FILE)
        }
    }

    // 压缩数据回调接口
    fun onCompressDataListener(listener: ((data: ByteArray, width: Int, height: Int) -> Unit)) {
        this.compressDataListener = listener
    }

    // 开始压缩线程
    private fun startCompressThread() {
        compressThread = thread(start = true) {

            // 循环执行
            while (isWork && !Thread.interrupted()) {
                // 暂停
                if (mPause.get()) continue
                // 原始相机数据
                val srcData = cameraDatas.take()
                // 保存压缩数据
                val compressData = ByteArray(scaleWidth * scaleHeight * 3 / 2)

                // 压缩
                LiveNativeManager.compressYUV(
                    srcData,
                    videoWidth,
                    videoHeight,
                    compressData,
                    scaleWidth,
                    scaleHeight,
                    0,
                    orientation,
                    orientation == 270
                )
                // 回调压缩数据
                compressDataListener?.let { it.invoke(compressData, scaleWidth, scaleHeight) }

                // 是否保存压缩数据
                if (SAVE_FILE_FOR_TEST) {
                    fileManager.saveFileData(compressData)
                }
            }
        }
    }

    fun switchCamera() {
        cameraSurface.switchCamera()
    }

    override fun init() {
        // 将视频数据压入队列
        cameraSurface.onCameraNVDataListener {
            it?.let {
                if (!mPause.get()) {
                    LogUtil.d("添加数据")
                    cameraDatas.put(it)
                }
            }
        }

        // 相机方向监听
        cameraSurface.onCameraOrientationChangeListener {
            LogUtil.d("摄像头方向：" + it)
            orientation = it
        }

        // 相机尺寸变换监听
        cameraSurface.onCameraSizeChangeListener { width, height ->
            LogUtil.d("摄像头宽高：" + width + "---" + height)
            videoWidth = width
            videoHeight = height
            scaleWidth = videoWidth / 3
            scaleHeight = scaleWidth * 75 / 100

            // 是否初始化了视频
            if (!isVideoInit) {
                LogUtil.d("初始化视频编码器")
                // 初始化视频编码
                LiveNativeManager.videoEncoderinit(
                    videoWidth,
                    videoHeight,
                    scaleWidth,
                    scaleHeight,
                    cameraSurface.getCameraUtil().getCameraOrientation()
                )
                isVideoInit = true
            }
        }

        // 打开相机
        cameraSurface.openCamera()
    }

    override fun start() {
        // 开始收集数据并压缩
        mPause.set(false)
        isWork = true
        // 注册加速度传感器
        sensorManager.registerListener(
            this,
            sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
            SensorManager.SENSOR_DELAY_UI
        )
        startCompressThread()
    }

    override fun stop() {
        isWork = false
        compressThread.interrupt()
        cameraSurface.releaseCamera()
        sensorManager.unregisterListener(this)
        cameraDatas.clear()
        if (SAVE_FILE_FOR_TEST) {
            fileManager.closeFile()
        }

        LiveNativeManager.releaseVideo()
    }

    override fun pause() {
        mPause.set(true)
    }

    override fun resume() {
        mPause.set(false)
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






















