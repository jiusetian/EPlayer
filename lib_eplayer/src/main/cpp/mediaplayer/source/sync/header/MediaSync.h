
#ifndef EPLAYER_MEDIASYNC_H
#define EPLAYER_MEDIASYNC_H
#include "MediaClock.h"
#include "PlayerState.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"

#include "VideoDevice.h"

/**
 * 视频同步器
 */
class MediaSync : public Runnable {

public:
    MediaSync(PlayerState *playerState);

    virtual ~MediaSync();

    void reset();

    void start(VideoDecoder *videoDecoder, AudioDecoder *audioDecoder);

    void stop();

    // 设置视频输出设备
    void setVideoDevice(VideoDevice *device);

    // 设置帧最大间隔
    void setMaxDuration(double maxDuration);

    // 更新视频帧的计时器
    void refreshVideoTimer();

    // 更新音频时钟
    void updateAudioClock(double pts, double time);

    // 获取音频时钟与主时钟的差值
    double getAudioDiffClock();

    // 更新外部时钟
    void updateExternalClock(double pts);

    double getMasterClock();

    void run() override;

    MediaClock *getAudioClock();

    MediaClock *getVideoClock();

    MediaClock *getExternalClock();

    long getCurrentPosition();

private:
    void refreshVideo(double *remaining_time);

    void checkExternalClockSpeed();

    double calculateDelay(double delay);

    double calculateDuration(Frame *vp, Frame *nextvp);

    void renderVideo();

private:
    PlayerState *playerState;               // 播放器状态
    bool abortRequest;                      // 停止
    bool mExit;

    MediaClock *audioClock;                 // 音频时钟
    MediaClock *videoClock;                 // 视频时钟
    MediaClock *extClock;                   // 外部时钟

    VideoDecoder *videoDecoder;             // 视频解码器
    AudioDecoder *audioDecoder;             // 视频解码器

    Mutex mMutex;
    Condition mCondition;
    Thread *syncThread;                     // 同步线程

    int forceRefresh;                       // 强制刷新标志
    double maxFrameDuration;                // 最大帧延时
    int frameTimerRefresh;                  // 刷新时钟
    double frameTimer;                      // 视频时钟

    VideoDevice *videoDevice;               // 视频输出设备

    AVFrame *pFrameARGB;
    uint8_t *mBuffer;
    SwsContext *swsContext;
};

#endif //EPLAYER_MEDIASYNC_H
