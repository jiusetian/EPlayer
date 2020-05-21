//
// Created by Guns Roses on 2020/5/13.
//
#include <malloc.h>
#include <AndroidLog.h>
#include "YuvProcess.h"
#include "YuvUtil.h"

YuvProcess::YuvProcess() {
    //avQueue = new AVQueue();
    avQueue1 = new BlockQueue<AvData *>();
}

YuvProcess::~YuvProcess() {
    free(temp_i420_data_scale);
    free(temp_i420_data);
    free(temp_i420_data_rotate);
    // 阻塞队列
    if (avQueue1) {
        avQueue1->flush();
        delete avQueue1;
    }
}

void YuvProcess::init() {
    // i420临时数据缓存空间
    temp_i420_data = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * srcWidth * srcHeight * 3 / 2));
    // i420临时缩小数据的缓存
    temp_i420_data_scale = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * scaleWidth * scaleHeight * 3 / 2));
    temp_i420_data_rotate = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * scaleWidth * scaleHeight * 3 / 2));
}

// push av data into queue
void YuvProcess::putAvData(uint8_t *data, int len) {
    // 分配空间
    AvData *avData = static_cast<AvData *>(malloc(sizeof(AvData)));
    avData->data = data;
    avData->len = len;
    avData->type = VIDEO;
    avData->nalNums = 0;
    avData->nalSizes = NULL;
    if (avQueue1) {
        LOGD("YUV开始保存=%d", avData->len);
        avQueue1->putData(avData);
        LOGD("YUV保存成功");
    } else {
        free(avData);
    }
}

void YuvProcess::start() {

    if (avQueue1) {
        avQueue1->start();
    }

    mutex.lock();
    abortRequest = false;
    condition.signal();
    mutex.unlock();
    // 开始线程
    if (!yuvThread) {
        yuvThread = new Thread(this);
        yuvThread->start();
    }
}

void YuvProcess::resume() {
    mutex.lock();
    pauseRequest = false;
    condition.signal();
    mutex.unlock();
}

void YuvProcess::stop() {
    mutex.lock();
    abortRequest = true;
    condition.signal();
    mutex.unlock();

    if (avQueue1) {
        avQueue1->abort();
        avQueue1->flush();
    }

    // 注销线程
    if (yuvThread) {
        yuvThread->join();
        delete yuvThread;
        yuvThread = NULL;
    }
}

void YuvProcess::pause() {
    mutex.lock();
    pauseRequest = true;
    condition.signal();
    mutex.unlock();
}

void YuvProcess::flush() {

    if (avQueue1) {
        avQueue1->flush();
    }
}

// 线程执行
void YuvProcess::run() {
    // 开始处理YUV数据
    excuteProcessYUV();
}

void YuvProcess::setYuvCallback(YuvProcess::YuvCallback callback) {
    yuvCallback = callback;
}

void YuvProcess::excuteProcessYUV() {
    int scaleLen = scaleWidth * scaleHeight * 3 / 2;
    //AvData *videoData=(AvData *) malloc(sizeof(AvData));
    AvData *videoData = NULL;

    for (;;) {
        // 停止
        if (abortRequest) {
            break;
        }
        // 暂停
        if (pauseRequest) {
            continue;
        }

        LOGD("YUV开始取");
        // 获取原始av数据
        videoData = avQueue1->getData();
        if (videoData == NULL) {
            // 终止
            break;
        }

        LOGD("YUV取成功：%d", videoData->len);
        uint8_t *compressData = new uint8_t[scaleLen]; // 保存压缩结果
        // 压缩数据
        if (degress == 0 || degress == 180) { // 横屏
            compressYUV(videoData->data, srcWidth, srcHeight, compressData, scaleWidth, scaleHeight, filterMode,
                        degress,
                        degress == 270);
        } else { // 竖屏
            compressYUV(videoData->data, srcWidth, srcHeight, compressData, scaleHeight, scaleWidth, filterMode,
                        degress,
                        degress == 270);
        }
        LOGD("YUV压缩成功");
//        uint8_t *cpy = new uint8_t[scaleLen];
//        // 复制压缩数据
//        memcpy(cpy, compressData, scaleLen);
        // 回调
        yuvCallback(compressData, scaleLen);
        // 释放
        free(videoData); // 不再使用的时候需要free
        // delete []compressData;
        LOGD("YUV回调完毕");
    }

}

void YuvProcess::compressYUV(uint8_t *src, int width, int height,
                             uint8_t *dst, int dst_width, int dst_height, int mode,
                             int degree,
                             bool isMirror) {
    // nv21转化为i420(标准YUV420P数据) 这个temp_i420_data大小是和Src_data是一样的，是原始YUV数据的大小
    YuvUtil::nv12ToI420(src, width, height, temp_i420_data);

    // 进行缩放的操作，这个缩放，会把数据压缩
    YuvUtil::scaleI420(temp_i420_data, width, height, temp_i420_data_scale, dst_width, dst_height, mode);

    // 如果是前置摄像头，进行镜像操作
    if (isMirror) {
        // 先旋转后镜像
        YuvUtil::rotateI420(temp_i420_data_scale, dst_width, dst_height, temp_i420_data_rotate, degree);
        // 因为旋转的角度都是90和270，那后面的数据width和height是相反的
        YuvUtil::mirrorI420(temp_i420_data_rotate, dst_height, dst_width, dst);
    } else {
        // 只是旋转
        YuvUtil::rotateI420(temp_i420_data_scale, dst_width, dst_height, dst, degree);
    }
}

void YuvProcess::cropYUV(uint8_t *src, int width, int height,
                         uint8_t *dst, int dst_width, int dst_height, int left, int top) {

    int srcLen = width * height * 3 / 2;
    YuvUtil::cropI420(src, srcLen, width, height, dst, dst_width, dst_height, left, top);
}


void YuvProcess::setScaleWH(int width, int height) {
    scaleWidth = width;
    scaleHeight = height;
}

void YuvProcess::setSrcWH(int width, int height) {
    srcWidth = width;
    srcHeight = height;
}

void YuvProcess::setDegress(int degress) {
    this->degress = degress;
}

