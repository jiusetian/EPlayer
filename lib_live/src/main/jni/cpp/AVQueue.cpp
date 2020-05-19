//
// Created by Guns Roses on 2020/5/8.
//
#include "AVQueue.h"
#include <malloc.h>
#include "AndroidLog.h"

AVQueue::AVQueue() {
    // 初始化相关变量
    abort_request = 0;
    first = NULL;
    last = NULL;
    size = 0;
}

AVQueue::~AVQueue() {
    abort();
    flush();
}

int AVQueue::putData(AvData *data) {
    mMutex.lock();
    // 保存数据
    LOGD("开始存数据：%d", data->len);
    AVNode *node = NULL;
    // 终止
    if (abort_request) {
        return -1;
    }

    // 分配结构体内存
    node = static_cast<AVNode *>(malloc(sizeof(AVNode)));
    if (!node) {
        LOGD("存数据分配内存失败");
        return -1;
    }

    // 赋值node
    node->data = data;
    node->next = NULL;

    LOGD("链表中存nal数量=%d", data->nalNums);
    LOGD("链表中存数据的大小=%d", data->len);
    if (0<data->nalNums < 10) {
        for (int i = 0; i < data->nalNums; i++) {
            LOGD("链表中存nal大小：%d", data->nalSizes[i]);
        }
    }

    if (first == NULL) { // 如果first为null，则表示添加第一个元素
        // LOGD("添加到第一个元素");
        first = node;
    } else {
        // LOGD("添加到最后一个元素");
        // 接到最后一个元素后面
        last->next = node;
    }
    LOGD("链表中存数据first的大小=%d", first->data->len);
    // 更新最后一个元素
    last = node;
    size++;
    LOGD("链表中存数据first的大小1=%d", first->data->len);
    mCondition.signal();
    mMutex.unlock();
    // 保存成功
    return 1;
}

AvData* AVQueue::getData() {
    mMutex.lock();
    AVNode *node = NULL;
    AvData *data=NULL;

    for (;;) {
        // 终止
        if (abort_request) {
            break;
        }
        // 取链表中的头元素
        node = first;
        if (node != NULL) { // 不为null，代表链表不为空
            LOGD("链表中取数据first大小=%d", first->data->len);
            first = node->next; // 取出头元素，更新first
            // node的下一个元素为null，代表没有元素了
            if (first == NULL) {
                last = NULL;
            }
            size--;
            // 给参数av赋值
            LOGD("链表中node取nal数量=%d", node->data->nalNums);
            data = node->data; // data是一个指针

            LOGD("链表中取nal数量=%d", data->nalNums);
            LOGD("链表中取数据大小=%d", data->len);
            if (data->nalNums < 10) {
                for (int i = 0; i < data->nalNums; i++) {
                    LOGD("链表中取nal大小：%d", data->nalSizes[i]);
                }
            }
            // 跳出
            break;
        }else { // 没元素阻塞，不跳出循环
            LOGD("取数据阻塞");
            mCondition.wait(mMutex);
        }
    }
    mMutex.unlock();
    return data;
}

int AVQueue::getData(AvData **data) {
    return getData(data, true); //阻塞的取元素
}


void AVQueue::flush() {
    AVNode *node = nullptr, *node1 = nullptr;

    mMutex.lock();
    node = first;
    while (node) {
        node1 = node->next;
        // 释放元素
        free(node);
        // 如果下一个元素不为null，则继续释放
        node = node1;
    }

    first = NULL;
    last = NULL;
    size = 0;
    mCondition.signal();

    mMutex.unlock();

}

int AVQueue::getData(AvData **data, bool block) {

    int ret;
    AVNode *node = NULL;

    mMutex.lock();

    for (;;) {

        if (abort_request) {
            ret = -1;
            break;
        }

        // 取链表中的头元素
        node = first;

        if (node != NULL) { // 不为null，代表链表有元素
            first = node->next; // 取出头元素，更新first
            // node的下一个元素为null，代表没有元素了
            if (first == NULL) {
                last = NULL;
            }
            size--;
            // 给参数av赋值
            *data = node->data; // data是一个指针，*data是指针指向的值

            LOGD("链表中取nal数量=%d", (*data)->nalNums);
            if ((*data)->nalNums < 10) {
                for (int i = 0; i < (*data)->nalNums; i++) {
                    LOGD("链表中取nal大小：%d", (*data)->nalSizes[i]);
                }
            }
            ret = 1;
            // 跳出
            break;
        } else if (!block) { // 没有元素但不阻塞
            ret = 0;
            // 跳出
            break;
        } else { // 没元素阻塞，不跳出循环
            LOGD("取数据阻塞");
            mCondition.wait(mMutex);
        }
    }

    mMutex.unlock();
    return ret;
}

void AVQueue::abort() {
    mMutex.lock();
    abort_request = 1;
    // 如果队列正在阻塞，则通知
    mCondition.signal();
    mMutex.unlock();
}


int AVQueue::getSize() {
    return size;
}

// start代表可以存取了
void AVQueue::start() {
    mMutex.lock();
    abort_request = 0;
    mCondition.signal();
    mMutex.unlock();
}

