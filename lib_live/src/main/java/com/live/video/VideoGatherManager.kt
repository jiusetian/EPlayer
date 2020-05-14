package com.live.video

import android.content.Context
import android.content.Context.SENSOR_SERVICE
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.widget.Toast
import com.live.FileManager
import com.live.LiveConfig
import com.live.LiveNativeManager
import com.live.LogUtil
import java.util.concurrent.LinkedBlockingDeque
import kotlin.concurrent.thread

/**
 * Author：Alex
 * Date：2019/10/4
 * Note：
 */
class VideoGatherManager(val cameraSurface: CameraSurface, val context: Context) : SensorEventListener {


    companion object {
        // 是否保存视频
        private const val SAVE_FILE_FOR_TEST = false
    }

    // 相关参数设置
    private var cameraWidth = LiveConfig.cameraWidth
    private var cameraHeight = LiveConfig.cameraHeight
    private var scaleWidth = LiveConfig.scaleWidthVer
    private var scaleHeight = LiveConfig.scaleHeightVer
    private var cropWidht = LiveConfig.cropWidthVer
    private var cropHeight = LiveConfig.cropHeightVer
    private var cropStartX = LiveConfig.cropStartX
    private var cropStartY = LiveConfig.cropStartY

    // 第一次实例化的时候是不需要的
    private var initialized = false
    private var lastX = 0f
    private var lastY = 0f
    private var lastZ = 0f

    // 传感器需要，这边使用的是加速度传感器
    private val sensorManager: SensorManager
    // 阻塞线程安全队列，生产者和消费者
    private val mQueue = LinkedBlockingDeque<ByteArray>()
    // 工作线程
    private lateinit var workerThread: Thread
    private lateinit var fileManager: FileManager
    private var isLoop: Boolean = false

    // yuv数据回调接口
    private lateinit var yuvDataListener: ((data: ByteArray, width: Int, height: Int) -> Unit)

    init {
        // 将视频预览数据压入到队列中
        cameraSurface.setCameraNVDataListener { it?.let { mQueue.put(it) } }

        sensorManager = context.getSystemService(SENSOR_SERVICE) as SensorManager

        isParametersSupport()
        LogUtil.d("初始化宽高=" + scaleWidth + "///" + scaleHeight)
        if (SAVE_FILE_FOR_TEST) {
            fileManager = FileManager(FileManager.TEST_YUV_FILE)
        }
    }

    // 初始化配置参数
    private fun initConfig() {
        cameraWidth = LiveConfig.cameraWidth
        cameraHeight = LiveConfig.cameraHeight
        val orientation = LiveConfig.cameraOrientation
        if (orientation == 90 || orientation == 270) { //竖屏
            scaleWidth = LiveConfig.scaleWidthVer
            scaleHeight = LiveConfig.scaleHeightVer
            cropWidht = LiveConfig.cropWidthVer
            cropHeight = LiveConfig.cropHeightVer
        } else { // 横屏
            scaleWidth = LiveConfig.scaleWidthLand
            scaleHeight = LiveConfig.scaleHeightLand
            cropWidht = LiveConfig.cropWidthLand
            cropHeight = LiveConfig.cropHeightLand
        }
        cropStartX = LiveConfig.cropStartX
        cropStartY = LiveConfig.cropStartY
    }

    private fun initVideo() {
        initConfig() // 初始化相关配置
        LogUtil.d("初始化视频相关")
        // 初始化视频编码
        LiveNativeManager.encoderVideoinit(LiveConfig.cameraWidth, LiveConfig.cameraHeight, scaleWidth, scaleHeight)
    }

    private fun releaseJniVideo() {
        LiveNativeManager.releaseVideo()
    }

    fun setYuvDataListener(listener: ((data: ByteArray, width: Int, height: Int) -> Unit)) {
        this.yuvDataListener = listener
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
                val srcData = mQueue.take()
                // YV12和 NV12所占内存是12bits/Pixel,所以一帧原始图像大小为 mCameraUtil.getCameraWidth()*mCameraUtil.getCameraHeight()*3/2.
                // 生成 I420(YUV标准格式数据及YUV420P)目标数据，生成后的数据长度 width * height * 3 / 2
                val dstData = ByteArray(scaleWidth * scaleHeight * 3 / 2)
                // 摄像头方向，正常的手机方向，后置摄像头的时候为90，前置摄像头的时候为270
                val orientation = LiveConfig.cameraOrientation

                // LogUtil.d("摄像头角度="+orientation)
                // 压缩 NV21(YUV420SP)数据，元素数据位1080 * 1920，很显然这样的数据推流会很占用带宽，我们压缩成480 * 640 的YUV数据
                // 为啥要转化为YUV420P数据？因为是在为转化为 H264数据在做准备，NV21不是标准的，只能先通过转换，生成标准YUV420P数据
                // 然后把标准数据encode为H264流
                // 参数 mode为0代表压缩最快的
                /**
                 * compressYUV方法首先nv21转化为i420，然后进行scale缩放，最后进行 rotate旋转（如果orientation==270还要进行镜像）
                 * 1.nv21转化为i420，前后数据大小不变，只是存储数据格式改变了，方便进一步的处理
                 * 2.scale缩放，比如原来图像为1080*1920，scale以后变为480*640，大大缩小了，但是图像就没那么清晰了
                 * 3.rotate旋转，比如原来宽高比为640：480，旋转90度以后宽高比为480:640，注意观察下面的参数，其实cameraWidth：cameraHeight=1920:1080，
                 * scaleWidthVer：scaleHeightVer=480:640，所以在传参的时候scaleHeight作为宽传进去，而scaleWidth作为高传进去的，因为最后旋转以后，出来的图像宽高比才是480:640
                 */
                if (orientation == 0 || orientation == 180) { //横屏
                    LiveNativeManager.compressYUV(
                        srcData,
                        LiveConfig.cameraWidth,
                        LiveConfig.cameraHeight,
                        dstData,
                        scaleWidth,
                        scaleHeight,
                        0,
                        orientation,
                        orientation == 270
                    )
                } else { //竖屏，90°或270°
                    LiveNativeManager.compressYUV(
                        srcData,
                        LiveConfig.cameraWidth,
                        LiveConfig.cameraHeight,
                        dstData,
                        scaleHeight,
                        scaleWidth,
                        0,
                        orientation,
                        orientation == 270
                    )
                }

                // LogUtil.d("剪切前=" + dstData.size)
                // 进行YUV420P数据裁剪的操作，测试下这个借口，我们可以对数据进行裁剪，裁剪后的数据也是I420数据，我们采用的是libyuv库文件
                // 这个libyuv库效率非常高，这也是我们用它的原因,
                val cropData = ByteArray(cropWidht * cropHeight * 3 / 2)
                LiveNativeManager.cropYUV(
                    dstData,
                    scaleWidth,
                    scaleHeight,
                    cropData,
                    cropWidht,
                    cropHeight,
                    cropStartX,
                    cropStartY
                )
                // LogUtil.d("剪切后=" + cropData.size)
                // LogUtil.d("数据=" + orientation + "---" + cameraWidth + "////" + cameraHeight + "/////" + scaleWidthVer + "////" + scaleHeightVer + "////" + cropWidht + "/////" + cropHeightVer)
                // 自此，我们得到了YUV420P标准数据，这个过程实际上就是NV21转化为YUV420P数据
                // 注意，有些机器是NV12格式，只是数据存储不一样，我们一样可以用libyuv库的接口转化
                yuvDataListener?.let { it.invoke(cropData, cropWidht, cropHeight) }

                // 设置为true，我们把生成的YUV文件用播放器播放一下，看我们的数据是否有误，起调试作用
                if (SAVE_FILE_FOR_TEST) {
                    fileManager.saveFileData(cropData)
                }
            }
        }
    }

    /**
     * 参数是否支持
     */
    private fun isParametersSupport(): Boolean {
        if (cropStartX % 2 != 0 && cropStartY % 2 != 0) {
            Toast.makeText(context, "剪裁开始位置必须为偶数", Toast.LENGTH_SHORT)
            return false
        }
        if (cropStartX + cropWidht > scaleWidth || cropStartY + cropHeight > scaleHeight) {
            Toast.makeText(context, "剪裁区域超出范围", Toast.LENGTH_SHORT)
            return false
        }
        return true
    }

    fun startVideoGather() {
        // 打开摄像头
        cameraSurface.openCamera()
        // 注册加速度传感器
        sensorManager.registerListener(
            this,
            sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
            SensorManager.SENSOR_DELAY_UI
        )
        initVideo()
        initWorkThread()
    }

    fun changeCamera() {
        cameraSurface.changeCamera()
    }

    fun stopVideoGather() {
        cameraSurface.releaseCamera()
        sensorManager.unregisterListener(this)
        isLoop = false
        mQueue.clear()
        // workerThread.interrupt()
        // 释放jni层相关资源
        releaseJniVideo()
    }

    fun releaseIOStream() {
        if (SAVE_FILE_FOR_TEST) {
            fileManager.closeFile()
        }
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






















