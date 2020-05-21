//
// Created by Guns Roses on 2020/5/18.
//

#ifndef EPLAYER_BLOCKQUEUE_H
#define EPLAYER_BLOCKQUEUE_H

#include <queue>
#include "Mutex.h"
#include "Condition.h"

using namespace std;

template<typename T>
class BlockQueue {

private:
    Mutex mutex;
    Condition condition;
    queue<T> mQueue;
    bool abort_request= false; // 停止

public:
    BlockQueue() {
    }

    ~BlockQueue() {
        abort();
        flush();
    }

    // T是泛型
    int putData(const T data) {
        //mutex.lock();
        if (abort_request){
            return -1;
        }
        // 保存
        mQueue.push(data);
       // mutex.unlock();
        // 通知解除等待
        condition.signal();
        return 1;
    }

    // 终止
    void abort() {
        mutex.lock();
        abort_request = true;
        // 如果队列正在阻塞，则通知
        condition.signal();
        mutex.unlock();
    }

    // 获取队列的大小
    int getSize() {
        return mQueue.size();
    }

    // 清空队列
    void flush() {
        mutex.lock();
        while (!mQueue.empty()){
            mQueue.pop();
        }
        mutex.unlock();
    }

    // start代表可以存取了
    void start() {
        mutex.lock();
        abort_request = false;
        condition.signal();
        mutex.unlock();
    }

    T getData() {
      //  mutex.lock();
        // 当队列为空时，进行等待
        while (mQueue.empty()) {
            condition.wait(mutex);
        }
        // 获取头元素
        T front = mQueue.front();
        // 取出后丢弃
        mQueue.pop();
      //  mutex.unlock();
        return front;
    }

    size_t size() {
        mutex.lock();
        int num = mQueue.size();
        mutex.unlock();
        return num;
    }

};


#endif //EPLAYER_BLOCKQUEUE_H
