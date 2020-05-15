package com.live.video

/**
 * Author：Alex
 * Date：2019/9/21
 * Note：
 */
interface CameraSurfaceListener {
    fun startAutoFocus(x: Float, y: Float)
    fun openCamera()
    fun releaseCamera()
    fun switchCamera():Int
}