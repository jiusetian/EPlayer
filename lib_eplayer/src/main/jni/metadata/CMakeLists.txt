# 添加头文件路径
#include_directories( source )
# 添加源文件
add_library(metadata_retriever

        SHARED

        # library
        MediaMetadataRetriever.cpp
        Metadata.cpp
        com_eplayer_EMeiaMetadataRetriever.cpp
        )

# 链接静态库
target_link_libraries(metadata_retriever

#        ffmpeg
        #ffmpeg库
        avutil
        swresample
        swscale
        avcodec
        avformat
        avfilter
        avdevice

        android
        #基于 C/C++ 的接口，可以访问 Android 中的 Bitmap 的像素缓冲区（bitmap buffers）
        jnigraphics
        log)

