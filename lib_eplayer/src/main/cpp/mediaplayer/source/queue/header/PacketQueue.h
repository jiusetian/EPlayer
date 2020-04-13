
#ifndef EPLAYER_PACKETQUEUE_H
#define EPLAYER_PACKETQUEUE_H

#include <queue>
#include "Mutex.h"
#include "Condition.h"

extern "C" {
#include "libavcodec/avcodec.h"
};

typedef struct PacketList {
    AVPacket pkt;
    //此结构体包含一个pkt和下一个pkt的指针
    struct PacketList *next;
} PacketList;

/**
 * 备注：这里不用std::queue是为了方便计算队列占用内存和队列的时长，在解码的时候要用到
 */
class PacketQueue {
public:
    PacketQueue();

    virtual ~PacketQueue();

    // 入队数据包
    int pushPacket(AVPacket *pkt);

    // 入队空数据包
    int pushNullPacket(int stream_index);

    // 刷新
    void flush();

    // 终止
    void abort();

    // 开始
    void start();

    // 获取数据包
    int getPacket(AVPacket *pkt);

    // 获取数据包
    int getPacket(AVPacket *pkt, int block);

    int getPacketSize();

    int getSize();

    int64_t getDuration();

    int isAbort();

private:
    int put(AVPacket *pkt);

private:
    Mutex mMutex;
    Condition mCondition;
    PacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
};

#endif //EPLAYER_PACKETQUEUE_H
