//
// Created by lxr on 2019/9/19.
//

#ifndef EPLAYER_RTMPLIVEPUSH_H
#define EPLAYER_RTMPLIVEPUSH_H

#include "Thread.h"
#include "AVQueue.h"

extern "C" {
#include <librtmp/rtmp.h>
};

class RtmpLivePush : public Runnable {

    typedef enum NalType {
        NAL_UNKNOWN = 0,
        NAL_SLICE = 1, /* 非关键帧 */
        NAL_SLICE_DPA = 2,
        NAL_SLICE_DPB = 3,
        NAL_SLICE_DPC = 4,
        NAL_SLICE_IDR = 5, /* 关键帧 */
        NAL_SEI = 6,
        NAL_SPS = 7, /* SPS帧 */
        NAL_PPS = 8,/* PPS帧 */
        NAL_AUD = 9,
        NAL_FILLER = 12,
    } NalType;

private:
    Mutex mutex;
    Condition condition;
    bool abortRequest; // 停止
    bool pauseRequest = false; // 暂停
    AVQueue *avQueue; // 存储视频数据
    Thread *rtmpThread; // yuv处理线程
    bool isSendAudioHeader=false;

public:
    unsigned char *rtmp_url;
    unsigned int start_time;

    RTMP *rtmp;

    RtmpLivePush();

    ~RtmpLivePush();

    void run() override;

    void start();

    void resume();

    void stop();

    void flush();

    void pause();

    void addAvData(AvData data);

    // push video data with rtmp
    void pushVideoData(AvData data);

    // push audio data with rtmp
    void pushAudioData(AvData data);

    void excuteRtmpPush();

    int getSampleRateIndex(int sampleRate);

    void init(unsigned char *url);

    void addSequenceAacHeader(int sampleRate, int channel,int timestamp);

    void addAccBody(unsigned char *buf, int len, long timeStamp);

    void addSequenceH264Header(unsigned char *sps, int sps_len, unsigned char *pps, int pps_len);

    void addH264Body(unsigned char *buf, int len, long timeStamp);

    void releaseRtmp();

    void pushSPSPPS(unsigned char *sps, int spsLen, unsigned char *pps, int ppsLen);

    int send_video_sps_pps(unsigned char *sps, int spsLen, unsigned char *pps, int ppsLen);
};

#endif //EPLAYER_RTMPLIVEPUSH_H
