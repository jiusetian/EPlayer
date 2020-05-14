//
// Created by Guns Roses on 2020/5/9.
//

#ifndef EPLAYER_MEDIAENCODER_H
#define EPLAYER_MEDIAENCODER_H

#include <Thread.h>
#include "AVQueue.h"

class MediaEncoder: public Runnable{

public:
    MediaEncoder();

    virtual ~MediaEncoder();

    virtual void start();

    virtual void stop();

    virtual void flush();

    virtual void pause();

    virtual void resume();

    virtual void run(); // 虚函数代表子类可以重写

protected:
    Mutex mutex;
    Condition condition;
    bool abortRequest; // 停止
    bool pauseRequest=false; // 暂停

    AVQueue *avQueue;
};


#endif //EPLAYER_MEDIAENCODER_H
