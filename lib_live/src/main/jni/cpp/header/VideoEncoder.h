//
// Created by lxr on 2019/9/19.
//

#ifndef EPLAYER_VIDEOENCODER_H
#define EPLAYER_VIDEOENCODER_H

#include <stdio.h>

extern "C" {
#include <libx264/x264.h>
}

class VideoEncoder{

private:
    //
    int in_width;
    int in_height;
    int out_width;
    int out_height;

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

public:
    VideoEncoder();
    ~VideoEncoder();

    bool open();
    /* encode the given data */
    int encodeFrame(char* inBytes, int frameSize, int pts, char* outBytes, int *outFrameSize);
    /* close the encoder and file, frees all memory */
    bool close();
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
    int getOutHeight() const;
    void setOutHeight(int outHeight);
    int getOutWidth() const;
    void setOutWidth(int outWidth);
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
};

#endif //EPLAYER_VIDEOENCODER_H
