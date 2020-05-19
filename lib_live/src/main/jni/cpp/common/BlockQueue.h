//
// Created by Guns Roses on 2020/5/18.
//

#ifndef EPLAYER_BLOCKQUEUE_H
#define EPLAYER_BLOCKQUEUE_H

#include <queue>
#include "Mutex.h"
#include "Condition.h"
using namespace std;

template <typename T>
class BlockQueue{

private:
    Mutex mutex;
    Condition condition;

    queue<T> _queue;

public:
    BlockQueue(){}

    ~BlockQueue(){}

    // &代表 T是引用类型
    void put(const T &data){
        mutex.lock();
        _queue.push(data);
        mutex.unlock();
        condition.signal();
    }


    T take() {
        mutex.lock();
        // 当队列为空时，进行等待
        while(_queue.empty()) {
            condition.wait(mutex);
        }
        T front = _queue.front();
        _queue.pop();
        mutex.unlock();
        return front;
    }

    size_t size() {
        mutex.lock();
        int num = _queue.size();
        mutex.unlock();
        return num;
    }

};

#endif //EPLAYER_BLOCKQUEUE_H
