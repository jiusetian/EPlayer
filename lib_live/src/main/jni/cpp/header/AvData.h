//
// Created by Guns Roses on 2020/5/9.
//

#ifndef EPLAYER_AVDATA_H
#define EPLAYER_AVDATA_H

// av数据的结构体
typedef struct AvData {
    int len; //数据长度
    uint8_t *data; // byte类型数据指针
    int type; // 数据类型
    int nalNums; // nal数量
    // int数组的指针
    int *nalSizes; // 保存nal的大小
} AvData;

typedef enum AvDataType {
    AUDIO = 0, // audio
    VIDEO = 1, // video

} AvDataType;


#endif //EPLAYER_AVDATA_H
