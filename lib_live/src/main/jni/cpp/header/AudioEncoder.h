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

public:
    AudioEncoder(int channels, int sampleRate, int bitRate);

    ~AudioEncoder();

    int init();

    int encodeAudio(unsigned char *inBytes, int length, unsigned char *outBytes, int outlength);

    // 开始音频编码
    int startEncodeAudio();

    int encodeWAVAudioFile();

    int encodePCMAudioFile();

    bool close();
};


#endif //EPLAYER_AUDIOENCODER_H
