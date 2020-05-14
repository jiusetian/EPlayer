//
// Created by Guns Roses on 2020/5/9.
//

#ifndef EPLAYER_AVDATA_H
#define EPLAYER_AVDATA_H

// av数据的结构体
typedef struct AvData {
    int len;
    uint8_t *data;
    int type;
    int nalNums; // nal数量
    int *nalSizes; // 保存nal的大小
} AvData;


#endif //EPLAYER_AVDATA_H
