
#ifndef EPLAYER_AVMESSAGEQUEUE_H
#define EPLAYER_AVMESSAGEQUEUE_H

#include "../../../../common/Mutex.h"
#include "../../../../common/Condition.h"
#include <cstring>
#include <assert.h>

extern "C" {
#include "../../../../include/libavutil/mem.h"
};

#include "../../player/header/PlayerMessage.h"

typedef struct AVMessage {
    int what;
    int arg1;
    int arg2;
    void *obj;

    void (*free)(void *obj);

    struct AVMessage *next;
} AVMessage;

inline static void message_init(AVMessage *msg) {
    memset(msg, 0, sizeof(AVMessage));
}

inline static void message_free(void *obj) {
    av_free(obj);
}

inline static void message_free_resouce(AVMessage *msg) {
    if (!msg || !msg->obj) {
        return;
    }
    assert(msg->free);
    msg->free(msg->obj);
    msg->obj = NULL;
}

class AVMessageQueue {
public:
    AVMessageQueue();

    virtual ~AVMessageQueue();

    void start();

    void stop();

    void flush();

    void release();

    void postMessage(int what);

    void postMessage(int what, int arg1);

    void postMessage(int what, int arg1, int arg2);

    void postMessage(int what, int arg1, int arg2, void *obj, int len);

    int getMessage(AVMessage *msg);

    int getMessage(AVMessage *msg, int block);

    void removeMessage(int what);

private:
    int putMessage(AVMessage *msg);

private:
    Mutex mMutex;
    Condition mCondition;
    AVMessage *mFirstMsg, *mLastMsg;
    bool abortRequest;
    int mSize;
};

#endif //EPLAYER_AVMESSAGEQUEUE_H
