//
// Created by Guns Roses on 2020/5/13.
//

#ifndef EPLAYER_YUVPROCESS_H
#define EPLAYER_YUVPROCESS_H

#include <Thread.h>
#include "AVQueue.h"

class YuvProcess : public Runnable {

    // 回调函数
    typedef void (YuvCallback)(uint8_t *data, int dataLen);

public:
    YuvProcess();

    ~YuvProcess();

    void run() override;

    void init();

    void start();

    void resume();

    void stop();

    void flush();

    void pause();

    // 开始处理YUV数据
    void excuteProcessYUV();

    void compressYUV(uint8_t *src_, int width, int height,
                     uint8_t *dst_, int dst_width, int dst_height, int mode,
                     int degree,
                     bool isMirror);

    void cropYUV(uint8_t *src_, int width, int height,
                 uint8_t *dst_, int dst_width, int dst_height, int left, int top);

    void setSrcWH(int width, int height);

    void setScaleWH(int width, int height);

    void setCropWH(int width, int height);

    void setStartXY(int cropStartX, int cropStartY);

    void setDegress(int degress);

    void setYuvCallback(YuvCallback callback);

private:

    Mutex mutex;
    Condition condition;
    bool abortRequest; // 停止
    bool pauseRequest=false; // 暂停
    AVQueue *avQueue; // 存储原始视频数据
    Thread *yuvThread; // yuv处理线程
    YuvCallback  *yuvCallback; // 回调函数

    // 临时空间
    uint8_t *temp_i420_data;
    uint8_t *temp_i420_data_scale;
    uint8_t *temp_i420_data_rotate;

    int srcWidth, srcHeight; // 原始数据宽高

    int scaleWidth, scaleHeight; // 压缩后的宽高

    int cropWidth, cropHeight; // 剪裁后的宽高

    int cropStartX, cropStartY; // 剪裁的起点

    int filterMode = 0; // 镜像类型

    int degress; // 旋转角度

    bool isMirror; // 是否镜像
};

#endif //EPLAYER_YUVPROCESS_H
