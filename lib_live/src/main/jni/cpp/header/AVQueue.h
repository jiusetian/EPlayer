//
// Created by Guns Roses on 2020/5/8.
//

#ifndef EPLAYER_AVQUEUE_H
#define EPLAYER_AVQUEUE_H

#include "Mutex.h"
#include "Condition.h"
#include "AvData.h"

// 节点结构体
typedef struct AVNode {
    AvData *data;
    struct AVNode *next;
} AVNode;

class AVQueue {

public:

    AVQueue();

    virtual ~AVQueue();

    // 数据入列
    int putData(AvData *data);

    // 取数据
    int getData(AvData **data);

    // 取数据
    AvData* getData();

    // 取数据
    int getData(AvData **data, bool block);

    int getSize();

    // 终止
    void abort();

    // 开始
    void start();

    // 清空队列
    void flush();


private:
    Mutex mMutex;
    Condition mCondition;
    // 链表的头和尾
    AVNode *first;
    AVNode *last;
    int size;
    int abort_request; // 停止

};


#endif //EPLAYER_AVQUEUE_H

























