package com.live.video

import android.app.Activity
import android.content.Context
import android.graphics.ImageFormat
import android.hardware.Camera
import android.view.Surface
import android.view.SurfaceHolder
import com.live.LiveConfig
import com.live.LogUtil


/**
 * Author：Alex
 * Date：2019/9/21
 * Note：
 */
@SuppressWarnings("deprecation")
class CameraUtil(val context: Context) {

    private lateinit var camera: Camera
    private var isFocusing = false //是否正在对焦
    private var isStartPreview = false //是否已经开始预览

    /**
     * 摄像头的旋转角度
     */
    private var orientation: Int = 0

    fun getMorientation(): Int = orientation

    //预览的一个摄像头画面的大小，默认1080p，如果不支持再选择其他的
    private var cameraWidth = LiveConfig.cameraWidth
    private var cameraHeight = LiveConfig.cameraHeight

    fun getCameraWidth() = cameraWidth
    fun getCameraHeight() = cameraHeight

    private var currentCameraType = Camera.CameraInfo.CAMERA_FACING_BACK //默认后置摄像头

    fun setCurrentCameraType(currentCameraType: Int) {
        this.currentCameraType = currentCameraType
    }

    fun getCurrentCameraType() = currentCameraType

    //选择合适的预览尺寸
    private fun choosePreviewSize(params: Camera.Parameters, width: Int, height: Int) {

        var maxSize: Camera.Size? = null

        for (size in params.supportedPreviewSizes) {
            LogUtil.d("宽高=" + size.width + "///" + size.height)
            //选择满足期望比例的尺寸
            if ((size.width / size.height == cameraWidth / cameraHeight) && size.width <= cameraWidth) {
                if (maxSize == null) {
                    maxSize = size
                } else if (maxSize.width < size.width) {
                    maxSize = size
                }

                //如果size刚好是期望的宽高时，则结束循环
                if (size.width == width && size.height == height) {
                    params.setPreviewSize(width, height)
                    return
                }
            }
        }

        if (maxSize != null) { //如果存在
            params.setPreviewSize(maxSize.width, maxSize.height)
        } else { //如果没有期望的尺寸，则使用推荐尺寸
            val preferredSize = params.preferredPreviewSizeForVideo
            params.setPictureSize(preferredSize.width, preferredSize.height)
        }
    }

    private fun getCameraDisplayOrientation(cameraId: Int): Int {
        //获取相机信息
        val info = Camera.CameraInfo();
        Camera.getCameraInfo(cameraId, info)

        /*正常的竖屏，rotation为Surface.ROTATION_0，当逆时针翻转90°的时候，rotation为Surface.ROTATION_90*/
        val rotation = (context as Activity).windowManager.defaultDisplay.rotation
        var degrees = 0
        when (rotation) {
            Surface.ROTATION_0 -> degrees = 0
            Surface.ROTATION_90 -> degrees = 90
            Surface.ROTATION_180 -> degrees = 180
            Surface.ROTATION_270 -> degrees = 270
        }

        //info.orientation手机正常方向并且为后置摄像头，值为90，前置摄像头，值为270
        var result: Int
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) { //前置摄像头
            //前置摄像头需要镜像,转化后进行设置
            result = (info.orientation + degrees) % 360
            camera.setDisplayOrientation((360 - result) % 360)
        } else {
            //如果正常的手机后置摄像头方向，此时degrees是0，后置摄像头的orientation是90，所以这里result是90
            result = (info.orientation - degrees + 360) % 360
            //后置摄像头直接进行显示
            camera.setDisplayOrientation(result)
        }

        return result
    }

    /**
     * 打开摄像头
     */
    fun openCamera(cameraType: Int) {
        camera = Camera.open()
        //先释放摄像头
        camera?.let { releaseCamera() }

        val info = Camera.CameraInfo()
        var cameraId = 0 //摄像头ID
        val numCameras = Camera.getNumberOfCameras()

        //找到匹配的摄像头并且打开
        for (i in 0..numCameras) { //不包括numCameras
            Camera.getCameraInfo(i, info)
            if (info.facing == cameraType) {
                cameraId = i
                //打开摄像头
                camera = Camera.open(i)
                currentCameraType = cameraType
                break
            }
        }

        if (camera == null) {
            throw RuntimeException("打开摄像头失败")
        }

        //设置摄像头旋转角度
        orientation = getCameraDisplayOrientation(cameraId)
        LiveConfig.cameraOrientation = orientation
        //获取当前相机参数
        val parameters = camera.parameters
        //选择预览尺寸
        choosePreviewSize(parameters, cameraWidth, cameraHeight)
        //对焦模式
        val focusModes = parameters.supportedFocusModes

        //自动对焦
        if (focusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
            parameters.focusMode = Camera.Parameters.FOCUS_MODE_AUTO
        } else if (focusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
            parameters.focusMode = Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO
        }
        //预览模式
        parameters.previewFormat = ImageFormat.NV21
        //设置摄像头参数
        camera.parameters = parameters
        /**
         * 请注意这个地方, camera返回的图像并不一定是默认设置的16:9（因为可能并不支持）
         * 所以重新设置一下摄像头的宽高
         */
        val size = camera.parameters.previewSize
        cameraWidth = size.width
        cameraHeight = size.height
        //设置宽高相关的配置
//        if (orientation == 0 || orientation == 180) { //横屏,宽大于高
//            LogUtil.d("设置摄像头宽高")
//            var tempW = LiveConfig.scaleWidthVer
//            var tempH = LiveConfig.scaleHeightVer
//            LiveConfig.scaleWidthVer = if (tempW > tempH) tempW else tempH
//            LiveConfig.scaleHeightVer = if (tempW > tempH) tempH else tempW
//            tempW=LiveConfig.cropWidthVer
//            tempH=LiveConfig.cropHeightVer
//            LiveConfig.cropWidthVer = if (tempW > tempH) tempW else tempH
//            LiveConfig.cropHeightVer = if (tempW > tempH) tempH else tempW
//        }else{ //竖屏，宽小于高
//            LogUtil.d("设置摄像头宽高")
//            var tempW = LiveConfig.scaleWidthVer
//            var tempH = LiveConfig.scaleHeightVer
//            LiveConfig.scaleWidthVer = if (tempW > tempH) tempH else tempW
//            LiveConfig.scaleHeightVer = if (tempW > tempH) tempW else tempH
//            tempW=LiveConfig.cropWidthVer
//            tempH=LiveConfig.cropHeightVer
//            LiveConfig.cropWidthVer = if (tempW > tempH) tempH else tempW
//            LiveConfig.cropHeightVer = if (tempW > tempH) tempW else tempH
//        }
        LogUtil.d("设置摄像头角度宽高=" + orientation + "///" + cameraWidth + "///" + cameraHeight)
        LiveConfig.apply {
            cameraWidth = size.width
            cameraHeight = size.height
        }
    }

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

    fun handleCameraStartPreview(surfaceHolder: SurfaceHolder, callback: Camera.PreviewCallback) {
        //设置摄像头预览回调，用于回调摄像头捕捉的帧数据
        camera.setPreviewCallback(callback)

        //设置摄像头渲染画面
        try {
            camera.setPreviewDisplay(surfaceHolder)
        } catch (e: Exception) {
            e.printStackTrace()
        }

        //开始捕捉画面并且渲染到预览画面中
        camera.startPreview()
        isStartPreview = true
        isFocusing = false
        //进行一次自动对焦
        startAutoFocus()
    }

    fun releaseCamera() {
        camera?.let {
            it.stopPreview()
            it.setPreviewCallback(null)
            it.release()
            isStartPreview = false
            isFocusing = false
        }
    }

}































