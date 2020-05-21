package com.live.video

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.animation.AnimatorSet
import android.animation.ObjectAnimator
import android.content.Context
import android.hardware.Camera
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.FrameLayout
import android.widget.ImageView
import com.lib_live.R
import com.live.common.LogUtil

/**
 * Author：Alex
 * Date：2019/9/21
 * Note：
 */
class CameraSurface : FrameLayout, SurfaceHolder.Callback, Camera.PreviewCallback,
    CameraSurfaceListener {

    // 相机信息的监听
    private lateinit var cameraListerner: CameraInfoListener

    private lateinit var surfaceView: SurfaceView
    private lateinit var ivFoucView: ImageView
    private lateinit var cameraUtil: CameraUtil
    private var targetAspect: Float = -1.0f


    constructor(context: Context) : super(context) {
        init()
    }

    constructor(context: Context, attributeSet: AttributeSet) : super(context, attributeSet) {
        init()
    }

    constructor(context: Context, attributeSet: AttributeSet, i: Int) : super(context, attributeSet, i) {
        init()
    }

    fun init() {
        val view = View.inflate(context, R.layout.layout_camera, null)
        surfaceView = view.findViewById(R.id.surface)
        ivFoucView = view.findViewById(R.id.iv_focus)
        removeAllViews()
        addView(view) // 添加摄像头预览控件

        // 创建所需要的camera和surfaceview
        cameraUtil = CameraUtil(context)
        // surfaceView的监听
        surfaceView.holder.addCallback(this)
    }

    fun getCameraUtil() = cameraUtil

    fun setCameraInfoListener(cameraListerner: CameraInfoListener) {
        this.cameraListerner = cameraListerner
    }


    /**
     * 摄像头预览帧回调
     */
    override fun onPreviewFrame(data: ByteArray, camera: Camera) {
        camera.addCallbackBuffer(data)
        // 如果是1080*1920像素的摄像头，那么一帧图像的大小为1080*1920*1.5=3110400 byte
        // LogUtil.d("原始摄像头数据="+data.size)
        // LogUtil.d("摄像头宽高="+camera.parameters.previewSize.width+"///"+camera.parameters.previewSize.height)
        // 获取NV数据
        cameraListerner?.let { it.onCameraNVDataListener(data) }
    }


    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {
    }

    override fun surfaceDestroyed(holder: SurfaceHolder?) {
        LogUtil.d("调用了Surface销毁")
        //cameraUtil.releaseCamera()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        // 打开摄像头
        //openCamera()
        // 设置摄像头接口和surfaceview的关联还有摄像头的数据回调相关
        // 第二个参数是摄像头的回调接口，回调方法为onPreviewFrame，这个方法可以捕捉到摄像头数据byte[] data,这就是摄像头的原始数据流，即YUV420SP格式的数据
        cameraUtil.handleCameraStartPreview(holder, this)
    }


    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        var widthMeasureSpec = widthMeasureSpec
        var heightMeasureSpec = heightMeasureSpec
        if (targetAspect > 0) {
            // 获取当前view的宽高值
            var initialWidth = MeasureSpec.getSize(widthMeasureSpec)
            var initialHeight = MeasureSpec.getSize(heightMeasureSpec)
            // 边距
            val hPadding = paddingLeft + paddingRight
            val vPadding = paddingBottom + paddingTop
            initialWidth -= hPadding
            initialHeight -= vPadding
            // 当前view的宽高比
            val viewAspectRatio = initialWidth / initialHeight
            // 目标比例和当前view比例的差值
            val aspectDiff = targetAspect / viewAspectRatio - 1

            if (Math.abs(aspectDiff) > 0.01) { // 差值大于0.01，则需要调整

                if (aspectDiff > 0) { // 当前view高的比例大于目标高的比例，所以要缩短高来适配mTargetAspect
                    initialHeight = (initialWidth / targetAspect).toInt()
                } else {
                    initialWidth = (initialHeight * targetAspect).toInt()
                }
                initialHeight += vPadding
                initialWidth += hPadding
                widthMeasureSpec = MeasureSpec.makeMeasureSpec(initialWidth, MeasureSpec.EXACTLY)
                heightMeasureSpec = MeasureSpec.makeMeasureSpec(initialHeight, MeasureSpec.EXACTLY)
            }
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (event.pointerCount == 1 && event.action == MotionEvent.ACTION_DOWN) {
            startAutoFocus(event.getX(), event.getY())
        }
        return true
    }

    override fun startAutoFocus(x: Float, y: Float) {
        // 后置摄像头才有对焦功能
        if (cameraUtil != null && cameraUtil.getCurrentCameraType() == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            return
        }
        if (x != -1f && y != -1f) {
            // 设置位置和初始状态
            ivFoucView.apply {
                translationX = x - width / 2
                translationY = y - height / 2
                clearAnimation()
            }

            // 执行动画
            val scaleX = ObjectAnimator.ofFloat(ivFoucView, "scaleX", 1.75f, 1.0f)
            val scaleY = ObjectAnimator.ofFloat(ivFoucView, "scaleY", 1.75f, 1.0f)
            // 动画设置
            AnimatorSet().apply {
                play(scaleX).with(scaleY)
                duration = 500
                addListener(object : AnimatorListenerAdapter() {
                    override fun onAnimationStart(animation: Animator) {
                        ivFoucView.visibility = View.VISIBLE
                    }

                    override fun onAnimationEnd(animation: Animator) {
                        ivFoucView.visibility = View.GONE
                    }
                })
                // 开始动画
                start()
            }

        }
        cameraUtil.startAutoFocus()
    }

    override fun openCamera() {
        //  打开摄像头
        cameraUtil.openCamera(cameraUtil.getCurrentCameraType())

        cameraListerner?.let { it.onCameraOrientationChangeListener(cameraUtil.getCameraOrientation()) }

        cameraListerner?.let {
            it.onCameraSizeChangeListener(
                cameraUtil.getCameraWidth(),
                cameraUtil.getCameraHeight()
            )
        }

        // 等待surfaceview创建好以后会执行post里面的内容
        surfaceView.post {
            cameraUtil.handleCameraStartPreview(surfaceView.holder, this)
            // 这里可以获取真正的预览的分辨率，在这里要进行屏幕的适配，主要适配非16:9的屏幕
            targetAspect = (cameraUtil.getCameraHeight() / cameraUtil.getCameraWidth()).toFloat()
            // 再次测量
            this.measure(-1, -1)
        }
    }

    // 改变摄像机方向
    fun changeCarmeraOrientation(): Int {
        val or = cameraUtil.changeCarmeraOrientation()
        cameraListerner?.let { it.onCameraOrientationChangeListener(cameraUtil.getCameraOrientation()) }
        return or
    }

    override fun releaseCamera() {
        cameraUtil.releaseCamera()
    }

    override fun switchCamera(): Int {
        releaseCamera()
        //  前置或者后置摄像头
        val cameraType = if (cameraUtil.getCurrentCameraType() == 0) 1 else 0
        //  摄像头类型
        cameraUtil.setCurrentCameraType(cameraType)
        //  打开摄像头
        openCamera()

        return cameraUtil.getCurrentCameraType()
    }

    // 相机信息的相关监听
    interface CameraInfoListener {
        // 摄像头数据监听
        fun onCameraNVDataListener(data: ByteArray)

        // 摄像头方向的监听
        fun onCameraOrientationChangeListener(orientation: Int)

        // 摄像头尺寸变换的监听
        fun onCameraSizeChangeListener(width: Int, height: Int)
    }
}































