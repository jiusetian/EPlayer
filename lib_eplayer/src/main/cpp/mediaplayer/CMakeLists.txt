
# 根据API版本判断使用哪个版本的OpenGLES
if (${ANDROID_PLATFORM_LEVEL} LESS 12) #应该是Android8以下不支持2.0的
    message(FATAL_ERROR "OpenGL 2 is not supported before API level 11 (currently using ${ANDROID_PLATFORM_LEVEL}).")
    return()
elseif (${ANDROID_PLATFORM_LEVEL} LESS 18) #Android版本低于18将使用OpenGL 2.0
    add_definitions("-DDYNAMIC_ES3")
    set(GLES-lib GLESv2)
else () #Android 18开始支持OpenGL ES3.0
    set(GLES-lib GLESv3)
endif (${ANDROID_PLATFORM_LEVEL} LESS 11)

# 添加 soundtouch 动态库，../是上一级的意思
set(soundtouch_dir ../soundtouch)
#添加子目录，所以soundtouch是当前项目的子项目
add_subdirectory(${soundtouch_dir} soundtouch)

# 添加头文件路径，代表当前源文件可以直接使用下面路径下的头文件
#同时因为子目录继承了当前CMake，所以这个设置对子目录也是有效的
include_directories(
        source
        source/common
        source/convertor
        source/decoder/header
        source/device/header
        source/device/android/header
        source/player/header
        source/queue/header
        # 渲染包render的header
        source/render/header
        source/render/common/header
        source/render/filter/header
        source/render/filter/input/header
        source/render/filter/adjust/header
        source/render/filter/beauty/header
        source/render/filter/effect/header
        source/sync/header
        android/header
        ${soundtouch_dir})

#源文件的路径
file(GLOB my_source
        source/common/*.cpp
        source/convertor/*.cpp
        source/decoder/*.cpp
        source/device/*.cpp
        source/device/android/*.cpp
        source/queue/*.cpp
        source/render/*.cpp
        source/render/common/*.cpp
        source/render/filter/*.cpp
        source/render/filter/input/*.cpp
        source/render/filter/adjust/*.cpp
        source/render/filter/beauty/*.cpp
        source/render/filter/effect/*.cpp
        source/sync/*.cpp
        source/player/*.cpp
        android/*.cpp
        )

# 添加源文件
add_library(
        media_player
        SHARED
        # library
        ${my_source}
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
        #根据系统api版本来设置
        ${GLES-lib})

