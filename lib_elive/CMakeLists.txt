
cmake_minimum_required(VERSION 3.4.1)

file(GLOB my_source
        src/main/jni/cpp/*.cpp
        src/main/jni/cpp/*.c
        )

#CMake是一个高级的编译配置工具，它可以根据不同平台、不同的编译器，生成相应的Makefile或者vcproj项目
add_library(
        live-lib
        SHARED
        ${my_source}
)

set(distribution_DIR ../../../../libs)

add_library(faac
        STATIC
        IMPORTED)
set_target_properties(faac
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/armeabi-v7a/libfdk-aac.a)

add_library(rtmp
        STATIC
        IMPORTED)
set_target_properties(rtmp
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/armeabi-v7a/librtmp.a)

add_library(yuv
        STATIC
        IMPORTED)
set_target_properties(yuv
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/armeabi-v7a/libyuv.a)

add_library(x264
        STATIC
        IMPORTED)
set_target_properties(x264
        PROPERTIES IMPORTED_LOCATION
        ${distribution_DIR}/armeabi-v7a/libx264.a)

#添加头文件
include_directories(
        src/main/jni/include
        src/main/jni/cpp/common
        src/main/jni/cpp/header
)
#target_include_directories(src/main/jni/include)

find_library(
        log-lib
        log)

target_link_libraries(
        #原生库
        live-lib
        #需要链接的库，这样原生库才能使用被链接库的函数
        faac
        rtmp
        yuv
        x264
        ${log-lib})