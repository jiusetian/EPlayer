
#ifndef EPLAYER_MEDIADECODER_H
#define EPLAYER_MEDIADECODER_H

#include "../../../../common/AndroidLog.h"
#include "player/header/PlayerState.h"
#include "queue/header/PacketQueue.h"
#include "queue/header/FrameQueue.h"

class MediaDecoder : public Runnable {
public:
    MediaDecoder(AVCodecContext *avctx, AVStream *stream, int streamIndex, PlayerState *playerState);

    virtual ~MediaDecoder();

    virtual void start();

    virtual void stop();

    virtual void flush();

    int pushPacket(AVPacket *pkt);

    int getPacketSize();

    int getStreamIndex();

    AVStream *getStream();

    AVCodecContext *getCodecContext();

    int getMemorySize();

    int hasEnoughPackets();

    virtual void run();

protected:
    Mutex mMutex;
    Condition mCondition;
    bool abortRequest;
    PlayerState *playerState;
    PacketQueue *packetQueue;       // 数据包队列
    AVCodecContext *pCodecCtx;
    AVStream *pStream;
    int streamIndex;
};

#endif //EPLAYER_MEDIADECODER_H
