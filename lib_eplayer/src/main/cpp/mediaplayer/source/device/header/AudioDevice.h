
#ifndef EPLAYER_AUDIODEVICE_H
#define EPLAYER_AUDIODEVICE_H

#include "player/header/PlayerState.h"

// 音频PCM填充回调
//首先这是一个函数指针，指向一个返回void，参数如下所示的函数，然后用typedef给这个函数指针起一个别名为AudioPCMCallback
typedef void (*AudioPCMCallback)(void *userdata, uint8_t *stream, int len);

typedef struct AudioDeviceSpec {
    int freq;                   // 采样率
    AVSampleFormat format;      // 音频采样格式
    uint8_t channels;           // 声道
    uint16_t samples;           // 采样大小
    uint32_t size;              // 缓冲区大小
    AudioPCMCallback callback;  // 音频回调，取得音频的PCM数据
    void *userdata;             // 音频上下文
} AudioDeviceSpec;

class AudioDevice : public Runnable {
public:
    AudioDevice();

    virtual ~AudioDevice();

    virtual int open(const AudioDeviceSpec *desired, AudioDeviceSpec *obtained);

    virtual void start();

    virtual void stop();

    virtual void pause();

    virtual void resume();

    virtual void flush();

    virtual void setStereoVolume(float left_volume, float right_volume);

    virtual void run();
};

#endif //EPLAYER_AUDIODEVICE_H
