//
// Created by Guns Roses on 2020/5/9.
//
#include "MediaEncoder.h"

MediaEncoder::MediaEncoder() {
    avQueue = new AVQueue();
}

MediaEncoder::~MediaEncoder() {
    mutex.lock();
    // 是否avqueue
    if (avQueue) {
        avQueue->flush();
        delete avQueue;
        avQueue = NULL;
    }
    mutex.unlock();
}

void MediaEncoder::pause() {
    mutex.lock();
    pauseRequest=true;
    condition.signal();
    mutex.unlock();
}

void MediaEncoder::pushAvData(AvData data) {
    if (avQueue){
        avQueue->pushData(&data);
    }
}

void MediaEncoder::resume() {
    mutex.lock();
    pauseRequest=false;
    condition.signal();
    mutex.unlock();
}

void MediaEncoder::start() {
    if (avQueue) {
        avQueue->start();
    }
    mutex.lock();
    abortRequest = false;
    condition.signal();
    mutex.unlock();
}

void MediaEncoder::flush() {
    if(avQueue){
        avQueue->flush();
    }
}

void MediaEncoder::stop() {
    mutex.lock();
    abortRequest = true;
    condition.signal();
    mutex.unlock();
    if (avQueue) {
        avQueue->abort();
    }
}

void MediaEncoder::run() {
    // do nothing
}
