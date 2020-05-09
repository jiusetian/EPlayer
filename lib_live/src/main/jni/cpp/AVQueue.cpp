//
// Created by Guns Roses on 2020/5/8.
//
#include "AVQueue.h"
#include <malloc.h>
#include "AndroidLog.h"

AVQueue::AVQueue() {
    abort_action = 0;
    first = NULL;
    last = NULL;
    size = 0;
}

AVQueue::~AVQueue() {
    abort();
    flush();
}

int AVQueue::getData(AvData *data) {
    getData(data, true); //阻塞的取元素
}


void AVQueue::flush() {
    AVNode *node, *node1;

    mMutex.lock();
    node = first;
    while (node) {
        node1 = node->next;
        // 释放元素
        free(node);
        // 如果下一个元素不为null，则继续释放
        node = node1;
    }

    first=NULL;
    last=NULL;
    size=0;
    mCondition.signal();

    mMutex.unlock();

}

int AVQueue::getData(AvData *data, bool block) {

    int ret;
    AVNode *node;

    mMutex.lock();

    for (;;) {

        if (abort_action) {
            ret = -1;
            break;
        }

        // 取链表中的头元素
        node = first;

        if (node) { // 不为null，代表链表有元素
            first = node->next; // 取出头元素，所以要更新
            if (!first) { // 没有元素了
                last = NULL;
            }
            size--;

            // 给参数av赋值
            *data = node->data; // data是一个指针，*data是指针指向的值
            // 释放node指针指向的内存空间，就代表其他代码有机会改变此内存空间了，此内存空间就不再归 node指针控制了，而是给参数data指针控制了
            free(node);
            ret = 1;
            break;
        } else if (!block) { // 没有元素但不阻塞
            ret = 0;
            break;
        } else { // 没元素但阻塞
            mCondition.wait(mMutex);
        }
    }

    mMutex.unlock();
    return ret;
}

void AVQueue::abort() {
    mMutex.lock();
    abort_action=1;
    mCondition.signal();
    mMutex.unlock();
}

int AVQueue::pushData(AvData *data) {
    int ret;
    mMutex.lock();

    ret = putData(data);
    mCondition.signal();

    mMutex.unlock();

    // 插入失败
    if (ret < 0) {
        LOGD("插入数据失败");
    }

    return ret;
}

int AVQueue::putData(AvData *data) {

    AVNode *node;
    // 终止
    if (abort_action) {
        return -1;
    }

    // 分配结构体内存
    node = static_cast<AVNode *>(malloc(sizeof(AVNode)));
    if (!node) {
        return -1;
    }

    // 赋值node
    node->data = *data;
    node->next = NULL;

    if (!last) { // 添加第一个元素
        first = node;
    } else {
        // 接到最后一个元素后面
        last->next = node;
    }

    // 更新最后一个元素为当前node
    last = node;
    size++;
    return 0;
}

int AVQueue::getSize() {
    return size;
}

// start代表可以存取了
void AVQueue::start() {
    mMutex.lock();
    abort_action = 0;
    mCondition.signal();
    mMutex.unlock();
}

