
#ifndef EPLAYER_MEDIAPLAYER_H
#define EPLAYER_MEDIAPLAYER_H

#include "MediaClock.h"
#include "SoundTouchWrapper.h"
#include "PlayerState.h"
#include "AudioDecoder.h"
#include "VideoDecoder.h"

#if defined(__ANDROID__)
#include "SLESDevice.h"
#include "GLESDevice.h"
#else
#include <device/AudioDevice.h>
#include <device/VideoDevice.h>
#endif
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "MediaSync.h"
#include "convertor/AudioResampler.h"


class MediaPlayer : public Runnable {
public:
    MediaPlayer();

    virtual ~MediaPlayer();

    status_t reset();

    void setDataSource(const char *url, int64_t offset = 0, const char *headers = NULL);

    void setVideoDevice(VideoDevice *videoDevice);

    status_t prepare();

    status_t prepareAsync();

    void start();

    void pause();

    void resume();

    void stop();

    void seekTo(float timeMs);

    void setLooping(int looping);

    void setVolume(float leftVolume, float rightVolume);

    void setMute(int mute);

    void setRate(float rate);

    void setPitch(float pitch);

    int getRotate();

    int getVideoWidth();

    int getVideoHeight();

    long getCurrentPosition();

    long getDuration();

    int isPlaying();

    int isLooping();

    int getMetadata(AVDictionary **metadata);

    AVMessageQueue *getMessageQueue();

    PlayerState *getPlayerState();

    void pcmQueueCallback(uint8_t *stream, int len);


protected:
    void run() override;

private:

    // 通知出错
    void notifyErrorMsg(const char* msg);

    // 解封装
    int demux();

    // 准备解码器
    int prepareDecoder();

    // 开始解码
    void startDecode();

    // 打开多媒体设备
    int openMediaDevice();

    // 获取AV数据
    int getAvPackets();

    int startPlayer();

    // prepare decoder with stream_index
    int prepareDecoder(int streamIndex);

    // open an audio output device
    int openAudioDevice(int64_t wanted_channel_layout, int wanted_nb_channels,
                        int wanted_sample_rate);

private:
    Mutex mMutex;
    Condition mCondition;
    Thread *readThread;                     // 读数据包线程

    PlayerState *playerState;               // 播放器状态

    AudioDecoder *audioDecoder;             // 音频解码器
    VideoDecoder *videoDecoder;             // 视频解码器
    bool mExit;                             // state for reading packets thread exited if not

    // 解复用处理
    AVFormatContext *pFormatCtx;            // 解码上下文
    int64_t mDuration;                      // 文件总时长，单位毫秒
    int lastPaused;                         // 上一次暂停状态
    int eof;                                // 数据包读到结尾标志
    int attachmentRequest;                  // 视频封面数据包请求

    AudioDevice *audioDevice;               // 音频输出设备

    AudioResampler *audioResampler;         // 音频重采样器

    MediaSync *mediaSync;                   // 媒体同步器

};
#endif //EPLAYER_MEDIAPLAYER_H
