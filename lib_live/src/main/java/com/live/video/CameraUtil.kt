package com.live.video

import android.app.Activity
import android.content.Context
import android.content.res.Configuration
import android.graphics.ImageFormat
import android.hardware.Camera
import android.view.Surface
import android.view.SurfaceHolder
import com.live.LiveConfig
import com.live.LogUtil


/**
 * Author：Alex
 * Date：2019/9/21
 * Note：相机工具类，主要是打开摄像头，预览摄像头，对焦
 */
@SuppressWarnings("deprecation")
class CameraUtil(val context: Context) {

    // 摄像头实例
    private lateinit var camera: Camera
    private var isFocusing = false // 是否正在对焦
    private var isStartPreview = false // 是否已经开始预览

    // 摄像头的旋转角度
    private var orientation: Int = -1

    // 期望的摄像头大小，默认1080p，如果不支持再选择其他
    private val exceptCameraWidth = 1920
    private val exceptCameraHeight = 1080

    // 当前摄像头尺寸
    private var camaraWidth = 0
    private var camaraHeight = 0

    // 摄像头ID
    private var cameraId = 0

    // 摄像头类型
    private var currentCameraType = Camera.CameraInfo.CAMERA_FACING_BACK // 默认后置摄像头

    /**
     * 打开摄像头
     */
    fun openCamera(cameraType: Int) {
        LogUtil.d("打开摄像头")
        // 摄像头信息
        val info = Camera.CameraInfo()

        // 选择前置或后置摄像头
        val numCameras = Camera.getNumberOfCameras()

        for (i in 0..numCameras) {
            Camera.getCameraInfo(i, info)
            if (info.facing == cameraType) {
                cameraId = i
                // 打开摄像头
                camera = Camera.open(i)
                currentCameraType = cameraType
                break
            }
        }

        if (camera == null) {
            throw RuntimeException("打开摄像头失败")
        }

        // 设置摄像头方向
        orientation = setCameraDisplayOrientation(cameraId)

        // 当前相机参数
        val parameters = camera.parameters
        // 选择合适的预览尺寸
        chooseBestPreviewSize(parameters, exceptCameraWidth, exceptCameraHeight)
        // 对焦模式
        val focusModes = parameters.supportedFocusModes

        // 自动对焦
        if (focusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
            parameters.focusMode = Camera.Parameters.FOCUS_MODE_AUTO
        } else if (focusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
            parameters.focusMode = Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO
        }
        // 预览模式
        parameters.previewFormat = ImageFormat.NV21
        // 设置摄像头参数
        camera.parameters = parameters
        /**
         * 请注意这个地方, camera返回的图像并不一定是默认设置的16:9（因为可能并不支持）
         * 所以重新设置一下摄像头的宽高
         */
        val size = camera.parameters.previewSize

        // 摄像头预览大小
        camaraWidth = size.width
        camaraHeight = size.height

    }

    // 对焦
    fun startAutoFocus() {
        try {
            if (camera != null && !isFocusing && isStartPreview) {
                camera.cancelAutoFocus()
                isFocusing = true
                camera.autoFocus { _, _ ->
                    isFocusing = false
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    // 摄像头预览
    fun handleCameraStartPreview(surfaceHolder: SurfaceHolder, callback: Camera.PreviewCallback) {
        LogUtil.d("开始预览")
        // 设置摄像头预览回调，用于回调摄像头捕捉的帧数据
        camera.setPreviewCallback(callback)

        // 设置摄像头渲染画面
        try {
            camera.setPreviewDisplay(surfaceHolder)
        } catch (e: Exception) {
            e.printStackTrace()
        }

        // 开始捕捉画面并且渲染到预览画面中
        camera.startPreview()
        isStartPreview = true
        isFocusing = false
        // 进行一次自动对焦
        startAutoFocus()
    }

    /**
     * 选择最佳预览尺寸
     */
    private fun chooseBestPreviewSize(params: Camera.Parameters, expectWidth: Int, expectHeight: Int) {

        var bestSize: Camera.Size? = null // 最佳尺寸

        // 遍历支持的预览尺寸
        for (size in params.supportedPreviewSizes) {
            LogUtil.d("宽高=" + size.width + "///" + size.height)

            // 选择跟期望宽高比例相等，并且不大于期望宽高的尺寸
            if ((size.width / size.height == expectWidth / expectHeight) && size.width <= expectWidth) {

                if (bestSize == null) {
                    bestSize = size
                } else if (bestSize.width < size.width) { // 选择所有符合比例中最大的
                    bestSize = size
                }

                // 如果size刚好是期望的宽高时，则结束循环
                if (size.width == expectWidth && size.height == expectHeight) {
                    params.setPreviewSize(expectWidth, expectHeight)
                    return
                }
            }
        }

        if (bestSize != null) { // 如果期望尺寸存在
            params.setPreviewSize(bestSize.width, bestSize.height)
        } else { // 如果没有期望的尺寸，则使用推荐尺寸
            val preferredSize = params.preferredPreviewSizeForVideo
            params.setPictureSize(preferredSize.width, preferredSize.height)
        }
    }

    /**
     * 摄像头方向
     */
    private fun setCameraDisplayOrientation(cameraId: Int): Int {
        // 获取相机信息
        val info = Camera.CameraInfo();
        Camera.getCameraInfo(cameraId, info)

        /*正常的竖屏，rotation为Surface.ROTATION_0，当逆时针翻转90°的时候，rotation为Surface.ROTATION_90*/
        val rotation = (context as Activity).windowManager.defaultDisplay.rotation // 屏幕方向
        var degrees = 0
        when (rotation) {
            Surface.ROTATION_0 -> degrees = 0
            Surface.ROTATION_90 -> degrees = 90
            Surface.ROTATION_180 -> degrees = 180
            Surface.ROTATION_270 -> degrees = 270
        }

        // info.orientation手机正常方向并且为后置摄像头为90，前置摄像头为270
        var result: Int

        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) { // 前置摄像头
            // 前置摄像头需要镜像,转化后进行设置
            result = (info.orientation + degrees) % 360
            camera.setDisplayOrientation((360 - result) % 360)
        } else {
            // 如果正常的手机后置摄像头方向，此时degrees是0，后置摄像头的orientation是90，所以 result是90
            result = (info.orientation - degrees + 360) % 360
            // 后置摄像头直接进行显示
            camera.setDisplayOrientation(result)
        }

        return result
    }

    // 释放摄像头
    fun releaseCamera() {
        camera?.let {
            it.stopPreview()
            it.setPreviewCallback(null)
            it.release()
            isStartPreview = false
            isFocusing = false
        }
    }

    // 设置相机类型
    fun setCurrentCameraType(currentCameraType: Int) {
        this.currentCameraType = currentCameraType
    }

    fun getCameraOrientation() = orientation

    // 当前相机类型
    fun getCurrentCameraType() = currentCameraType

    // 摄像头宽高
    fun getCameraWidth() = camaraWidth
    fun getCameraHeight() = camaraHeight

    // 改变相机显示方向
    fun changeCarmeraOrientation(): Int = setCameraDisplayOrientation(cameraId)

}































