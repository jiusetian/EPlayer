# 设置cmake最低版本
cmake_minimum_required(VERSION 3.4.1)

# 设置GCC编译器的编译选项
if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes")
endif (CMAKE_COMPILER_IS_GNUCC)

# 设置clang编译器的编译选项
if (CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wmissing-prototypes -Wdocumentation -Wunreachable-code")
endif (CMAKE_COMPILER_IS_CLANG)

# 为了确保 CMake 可以在编译时定位头文件
# 这样就可以使用 #include <xx.h> 引入
# 否则需要使用 #include "path/xx"
# 添加include目录路径
#common和include为最外层的头文件，在这里加进去以后，所有当前CMake的子目录都可以直接使用这两个文件的头文件
#include_directories(common
#        include1)
#
## 添加ffmpeg库，IMPORTED指以导入的形式加入到项目中来，下面的set_target_properties是指定IMPORTED的路径
#add_library( ffmpeg SHARED IMPORTED )
#
##链接库文件，${CMAKE_SOURCE_DIR}指cmakeList.txt所在的文件夹路径， ${ANDROID_ABI}指构建的COU，../代表父级目录
#set_target_properties( ffmpeg
#        PROPERTIES IMPORTED_LOCATION
#        ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libffmpeg.so )


##相当于指定一个变量，后面可以通过${var}来引用，这里我们指定ffmpeg的库文件路径
set(JNI_LIBS_DIR ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})

##===========================================================================================================
#添加库文件
#工具库（大部分库都需要这个库的支持）

include_directories(common
        include)

add_library(
        avutil
        SHARED
        IMPORTED
)
set_target_properties(
        avutil
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/libavutil.so
)

#音频采样数据格式转换
add_library(swresample
        SHARED
        IMPORTED )

set_target_properties(swresample
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/libswresample.so )

#视频像素格式转换
add_library(swscale
        SHARED
        IMPORTED )

set_target_properties(swscale
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/libswscale.so )

#编解码（最重要的库）
add_library(avcodec
        SHARED
        IMPORTED )

set_target_properties(avcodec
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/libavcodec.so )

#封装格式处理
add_library(avformat
        SHARED
        IMPORTED )

set_target_properties(avformat
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/libavformat.so )

#滤镜特效处理
add_library(avfilter
        SHARED
        IMPORTED )

set_target_properties(avfilter
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/libavfilter.so )

#各种设备的输入输出
add_library(avdevice
        SHARED
        IMPORTED )
set_target_properties(avdevice
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/libavdevice.so )

#==============================================================================================================================

# Metadata库为子库
add_subdirectory(${CMAKE_SOURCE_DIR}/metadata)

#mediaplayer库为子库
add_subdirectory(${CMAKE_SOURCE_DIR}/mediaplayer)

#当前的目录关系为，当前CMake文件有两个子目录，分别是metadata的CMake和mediaplayer的CMake，而mediaplayer的子目录为soundtouch
#的CMake