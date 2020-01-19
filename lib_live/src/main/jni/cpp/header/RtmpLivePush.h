//
// Created by lxr on 2019/9/19.
//

#ifndef EPLAYER_RTMPLIVEPUSH_H
#define EPLAYER_RTMPLIVEPUSH_H

#include <pthread.h>
#include <jni.h>

extern "C" {
#include <librtmp/rtmp.h>
};

class RtmpLivePush {
public:
    unsigned char *rtmp_url;
    unsigned int start_time;

    RTMP *rtmp;

    RtmpLivePush();
    ~RtmpLivePush();

    int getSampleRateIndex(int sampleRate);

    void init(unsigned char *url);

    void addSequenceAacHeader(int timestamp, int sampleRate, int channel);

    void addAccBody(unsigned char *buf, int len, long timeStamp);

    void addSequenceH264Header(unsigned char *sps, int sps_len, unsigned char *pps, int pps_len);

    void addH264Body(unsigned char *buf, int len, long timeStamp);

    void release();

    void pushSPSPPS(unsigned char *sps, int spsLen,unsigned char *pps, int ppsLen);

    int send_video_sps_pps(unsigned char *sps, int spsLen, unsigned char *pps, int ppsLen);
};

#endif //EPLAYER_RTMPLIVEPUSH_H
