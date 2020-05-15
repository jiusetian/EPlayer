//
// Created by lxr on 2019/9/19.
//

#ifndef EPLAYER_VIDEOENCODER_H
#define EPLAYER_VIDEOENCODER_H

#include <stdio.h>
#include "MediaEncoder.h"

extern "C" {
#include <libx264/x264.h>
}

class VideoEncoder :public MediaEncoder{

    typedef void (videoEncoderCallback)(uint8_t* data,int len,int nalNum,int* nalsSize);

private:
    // 视频宽高
    int in_width;
    int in_height;

    int fps; //帧率
    int bitrate; //比特率
    int i_threads; //并行编码
    int i_vbv_buffer_size; //码率控制缓冲区的大小，单位Kbit，默认为0
    int i_slice_max_size; //每片字节的最大数，包括预计的NAL开销.
    int b_frame_frq; //B帧频率

    /*描述视频的特征的结构体*/
    x264_picture_t pic_in;
    x264_picture_t pic_out;
    /*编码参数结构体*/
    x264_param_t params;
    /*nal数组*/
    x264_nal_t* nals;
    /*nal数组大小*/
    int nal_nums;
    /*编码器*/
    x264_t* encoder;

    FILE *out1;
    FILE *out2;

    Thread *encoderThread; // 编码线程
    videoEncoderCallback* callback;

protected:
    // 线程执行函数
    void run() override;

    // 开始编码视频
    void excuteEncodeVideo();

public:
    VideoEncoder();
    ~VideoEncoder();

    bool open();
    /* encode the given data */
    int encodeFrame(uint8_t * inBytes, int frameSize, int pts, uint8_t * outBytes, int *outFrameSize);
    /* close the encoder and file, frees all memory */
    bool closeEncoder();
    /* validates if all params are set correctly, like width,height, etc.. */
    bool validateSettings();
    /* sets the x264 params */
    void setParams();
    void setParams2();
    int getFps() const;
    void setFps(int fps);
    int getInHeight() const;
    void setInHeight(int inHeight);
    int getInWidth() const;
    void setInWidth(int inWidth);
    int getNumNals() const;
    void setNumNals(int numNals);
    int getBitrate() const;
    void setBitrate(int bitrate);
    int getSliceMaxSize() const;
    void setSliceMaxSize(int sliceMaxSize);
    int getVbvBufferSize() const;
    void setVbvBufferSize(int vbvBufferSize);
    int getIThreads() const;
    void setIThreads(int threads);
    int getBFrameFrq() const;
    void setBFrameFrq(int frameFrq);
    // 设置回调函数
    void setVideoEncoderCallback(videoEncoderCallback* callback);

    void start() override;

    void stop() override;

    void flush() override;
};

#endif //EPLAYER_VIDEOENCODER_H
