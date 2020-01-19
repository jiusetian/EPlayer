//
// Created by public on 2019/9/19.
//

#ifndef EPLAYER_AUDIOENCODER_H
#define EPLAYER_AUDIOENCODER_H


extern "C" {
#include <fdk-aac/aacenc_lib.h>
}

class AudioEncoder {

private:
    int sampleRate;
    int channels;
    int bitRate;
    HANDLE_AACENCODER handle;

public:
    AudioEncoder(int channels, int sampleRate, int bitRate);
    ~AudioEncoder();
    int init();

    int encodeAudio(unsigned char* inBytes, int length, unsigned char* outBytes, int outlength);
    int encodeWAVAudioFile();
    int encodePCMAudioFile();
    bool close();
};


#endif //EPLAYER_AUDIOENCODER_H
