package com.live.avlive1.video

import android.content.Context
import android.content.Context.SENSOR_SERVICE
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import com.live.common.FileManager
import com.live.common.LiveInterfaces
import com.live.LiveNativeApi
import com.live.common.LogUtil
import java.util.concurrent.LinkedBlockingDeque
import kotlin.concurrent.thread

/**
 * Author：Alex
 * Date：2019/10/4
 * Note：
 */
class VideoGatherManager(val cameraSurface: CameraSurface, val context: Context) : SensorEventListener,
    LiveInterfaces, CameraSurface.CameraInfoListener {

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
    private var mPause = true

    // 第一次实例化的时候是不需要的
    private var initialized = false
    private var lastX = 0f
    private var lastY = 0f
    private var lastZ = 0f

    // 加速度传感器
    private val sensorManager: SensorManager

    // 相机数据
    private val cameraDatas = LinkedBlockingDeque<ByteArray>()

    // lateinit var相当于声明属性为 NotNull，如果属性还没初始化的时候，是不能使用的
    // 压缩数据的线程
    private var compressThread: Thread? = null
    private lateinit var fileManager: FileManager

    // 线程是否工作
    private var isLoop: Boolean = true

    // 压缩回调接口
    private lateinit var compressDataListener: ((data: ByteArray, width: Int, height: Int) -> Unit)

    init {
        sensorManager = context.getSystemService(SENSOR_SERVICE) as SensorManager
        if (SAVE_FILE_FOR_TEST) {
            fileManager =
                FileManager(FileManager.TEST_YUV_FILE)
        }
        // 相机信息的监听
        cameraSurface.setCameraInfoListener(this)
    }

    // 压缩数据回调接口
    fun onCompressDataListener(listener: ((data: ByteArray, width: Int, height: Int) -> Unit)) {
        this.compressDataListener = listener
    }

    // 开始压缩线程
    private fun startCompressThread() {
        compressThread = thread {

            // 循环执行
            while (isLoop && !Thread.interrupted()) {
                try {// 原始相机数据
                    val srcData = cameraDatas.take()
                    //LogUtil.d("压缩视频数据")
                    // 保存压缩数据
                    val compressData = ByteArray(scaleWidth * scaleHeight * 3 / 2)

                    // 压缩
                    LiveNativeApi.compressYUV(
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
                } catch (e: Exception) {
                    e.printStackTrace()
                }
            }
        }
    }

    fun switchCamera() {
        cameraSurface.switchCamera()
    }

    override fun init() {
        // 打开相机
        // cameraSurface.openCamera()
    }

    override fun start() {
        // 开始收集数据并压缩
        mPause = false
        isLoop = true
        // 注册加速度传感器
        sensorManager.registerListener(
            this,
            sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
            SensorManager.SENSOR_DELAY_UI
        )
        startCompressThread()
    }

    override fun stop() {
        isLoop = false
        compressThread?.let {
            it.interrupt()
        }
        //cameraSurface.releaseCamera()
        sensorManager.unregisterListener(this)
        cameraDatas.clear()
        if (SAVE_FILE_FOR_TEST) {
            fileManager.closeFile()
        }
    }

    override fun destrory() {
    }

    override fun pause() {
        mPause = true
    }

    override fun resume() {
        mPause = false
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
            // 传感器感应对焦，貌似对焦太频繁了
            //cameraSurface.startAutoFocus(-1f, -1f)
        }
        lastX = x
        lastY = y
        lastZ = z
    }

    override fun onCameraNVDataListener(data: ByteArray) {
        if (!mPause) cameraDatas.put(data)
    }

    override fun onCameraOrientationChangeListener(orientation: Int) {
        LogUtil.d("摄像头方向：" + orientation)
        this.orientation = orientation
    }

    override fun onCameraSizeChangeListener(width: Int, height: Int) {
        LogUtil.d("摄像头宽高：" + width + "---" + height)
        videoWidth = width
        videoHeight = height
        scaleWidth = videoWidth / 3
        scaleHeight = scaleWidth * 75 / 100

        // 是否初始化了视频
        if (!isVideoInit) {
            // 初始化视频编码
            LiveNativeApi.videoEncoderinit(
                videoWidth,
                videoHeight,
                scaleWidth,
                scaleHeight,
                cameraSurface.getCameraUtil().getCameraOrientation()
            )
            isVideoInit = true
        }
    }

}






















