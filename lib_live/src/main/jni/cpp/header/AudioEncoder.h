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
    typedef void (EncodeCallback)(uint8_t *data, int dataLen);

protected:
    // 线程执行函数
    void run() override;

    void start() override;

    void stop() override;

    void flush() override;

private:
    int sampleRate;
    int channels;
    int bitRate;
    HANDLE_AACENCODER handle;

    Thread *encoderThread; // 编码线程

    // 回调函数
    EncodeCallback *callback= nullptr;


public:
    AudioEncoder(int channels, int sampleRate, int bitRate);

    ~AudioEncoder();

    // 设置回调函数
    void setEncodeCallback(EncodeCallback *encodeCallback);

    int init();

    // 其实 uint8_t和 unsigned char 是一样的
    int encodeAudio(uint8_t *inBytes, int length, uint8_t *outBytes, int outlength);

    // 开始音频编码
    int startEncodeAudio();

    int encodeWAVAudioFile();

    int encodePCMAudioFile();

    bool close();
};


#endif //EPLAYER_AUDIOENCODER_H
