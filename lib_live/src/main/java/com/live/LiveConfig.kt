package com.live

/**
 * Author：Alex
 * Date：2019/9/23
 * Note：相关配置
 */
object LiveConfig {

    //相机宽高
    var cameraWidth = 1920
    var cameraHeight = 1080

    //竖屏缩放宽高
    var scaleWidthVer = 480
    var scaleHeightVer = 640
    //横屏缩放宽高
    var scaleWidthLand = 640
    var scaleHeightLand = 480

    //竖屏剪裁宽高
    var cropWidthVer = 480
    var cropHeightVer = 640
    //横屏剪裁宽高
    var cropWidthLand = 640
    var cropHeightLand = 480

    //剪裁开始的坐标
    var cropStartX = 0
    var cropStartY = 0

    const val SAMPLE_RATE = 44100
    const val CHANNELS = 2
    const val BIT_RATE = 64000

    //相机角度
    var cameraOrientation = 0

}