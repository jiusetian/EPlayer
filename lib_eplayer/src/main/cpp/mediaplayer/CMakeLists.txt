
# 根据API版本判断使用哪个版本的OpenGLES
if (${ANDROID_PLATFORM_LEVEL} LESS 12)
    message(FATAL_ERROR "OpenGL 2 is not supported before API level 11 (currently using ${ANDROID_PLATFORM_LEVEL}).")
    return()
elseif (${ANDROID_PLATFORM_LEVEL} LESS 18)
    add_definitions("-DDYNAMIC_ES3")
    set(GLES-lib GLESv2)
else ()
    set(GLES-lib GLESv3)
endif (${ANDROID_PLATFORM_LEVEL} LESS 11)

# 添加 soundtouch 动态库，../是上一级的意思
set(soundtouch_dir ../SoundTouch)
#添加子目录，所以soundtouch是当前项目的子项目
add_subdirectory(${soundtouch_dir} soundtouch)

# 添加头文件路径，第三个参数是子目录的路径，这里include了代表当前源文件可以直接使用子目录下的头文件
#同时因为子目录继承了当前CMake，所以从第一和第二个参数来看，子目录也是可以直接使用当前目录的头文件的
include_directories(
        source
        android
        ${soundtouch_dir})

# 添加源文件
add_library(
        media_player

        SHARED

        # library
        source/common/FFmpegUtils.cpp
        #
        source/convertor/AudioResampler.cpp
        #
        source/decoder/AudioDecoder.cpp
        source/decoder/MediaDecoder.cpp
        source/decoder/VideoDecoder.cpp
        #
        source/device/android/GLESDevice.cpp
        source/device/android/SLESDevice.cpp
        source/device/AudioDevice.cpp
        source/device/VideoDevice.cpp

        source/queue/FrameQueue.cpp
        source/queue/PacketQueue.cpp
        source/queue/AVMessageQueue.cpp
        #render
        source/renderer/EPlayerEGLContext.cpp
        source/renderer/helper/CoordinateUtils.cpp
        source/renderer/helper/EglHelper.cpp
        source/renderer/FrameBuffer.cpp
        source/renderer/GLFilter.cpp
        source/renderer/GLInputABGRFilter.cpp
        source/renderer/GLInputFilter.cpp
        source/renderer/GLInputYUV420PFilter.cpp
        source/renderer/InputRenderNode.cpp
        source/renderer/helper/OpenGLUtils.cpp
        source/renderer/RenderNode.cpp
        source/renderer/helper/vecmath.cpp
        #
        source/sync/MediaClock.cpp
        source/sync/MediaSync.cpp
        source/player/MediaPlayer.cpp
        source/player/PlayerState.cpp
        #
        # controller
        android/EMediaPlayer.cpp
        android/JNIHelp.cpp
        android/com.eplayer.EasyMediaPlayer.cpp
        )

# 链接静态库
target_link_libraries(
        #原生库
        media_player
        #需要链接的库
#        ffmpeg
        soundtouch
        #ffmpeg库
        avutil
        swresample
        swscale
        avcodec
        avformat
        avfilter
        avdevice

        -landroid
        -llog
        -lOpenSLES
        -lEGL
        ${GLES-lib})

