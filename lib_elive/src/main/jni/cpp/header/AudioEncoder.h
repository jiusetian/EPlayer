//
// Created by public on 2019/9/19.
//

#ifndef EPLAYER_AUDIOENCODER_H
#define EPLAYER_AUDIOENCODER_H

#include "Thread.h"
#include "MediaEncoder.h"

extern "C" {
#include <fdk-aac/aacenc_lib.h>
}

class AudioEncoder : public MediaEncoder {

    // 编码结果的回调函数
    typedef void (EncoderCallback)(uint8_t *data, int dataLen);

protected:
    // 线程执行函数
    void run() override;

private:
    int sampleRate;
    int channels;
    int bitRate;
    HANDLE_AACENCODER handle;

    Thread *encoderThread = nullptr; // 编码线程

    // 回调函数
    EncoderCallback *callback = nullptr;


public:
    AudioEncoder(int channels, int sampleRate, int bitRate);

    ~AudioEncoder();

    // 设置回调函数
    void setEncoderCallback(EncoderCallback *encodeCallback);

    int init();

    void start() override;

    void stop() override;

    void flush() override;

    void putAudioData(uint8_t *data, int len);


    // 其实 uint8_t和 unsigned char 是一样的
    int encodeAudio(uint8_t *inBytes, int length, uint8_t *outBytes, int outlength);

    // 开始音频编码
    int excuteEncodeAudio();

    int encodeWAVAudioFile();

    int encodePCMAudioFile();

    bool close();
};


#endif //EPLAYER_AUDIOENCODER_H
