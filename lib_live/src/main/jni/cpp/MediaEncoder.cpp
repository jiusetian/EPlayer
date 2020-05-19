//
// Created by Guns Roses on 2020/5/9.
//
#include "MediaEncoder.h"

MediaEncoder::MediaEncoder() {
    //avQueue = new AVQueue();
    avQueue1=new BlockQueue<AvData*>();
}

MediaEncoder::~MediaEncoder() {
    mutex.lock();
    // 是否avqueue
    if (avQueue1) {
        avQueue1->flush();
        delete avQueue1;
        avQueue1 = NULL;
    }

    mutex.unlock();
}

void MediaEncoder::pause() {
    mutex.lock();
    pauseRequest=true;
    condition.signal();
    mutex.unlock();
}

void MediaEncoder::putAvData(AvData* data) {
    if (avQueue1){
        avQueue1->putData(data);
    }

}

void MediaEncoder::resume() {
    mutex.lock();
    pauseRequest=false;
    condition.signal();
    mutex.unlock();
}

void MediaEncoder::start() {
    if (avQueue1) {
        avQueue1->start();
    }
    mutex.lock();
    abortRequest = false;
    condition.signal();
    mutex.unlock();
}

void MediaEncoder::flush() {
    if(avQueue1){
        avQueue1->flush();
    }
}

void MediaEncoder::stop() {
    mutex.lock();
    abortRequest = true;
    condition.signal();
    mutex.unlock();

    if (avQueue1) {
        avQueue1->abort();
        flush();
    }
}

void MediaEncoder::run() {
    // do nothing
}
