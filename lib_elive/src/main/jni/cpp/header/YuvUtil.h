//
// Created by lxr on 2019/10/16.
//

#ifndef EPLAYER_YUVUTIL_H
#define EPLAYER_YUVUTIL_H

#include <cstdint>

class YuvUtil {
public:
    // nv21 --> i420
    static void nv21ToI420(uint8_t *src_nv21_data, int width, int height, uint8_t *src_i420_data);

    // i420 --> nv21
    static void i420ToNv21(uint8_t *src_i420_data, int width, int height, uint8_t *src_nv21_data);

    // nv12 --> i420
    static void nv12ToI420(uint8_t *Src_data, int src_width, int src_height, uint8_t *Dst_data);

    // i420 --> nv12
    static void i420ToNv12(uint8_t *src_i420_data, int width, int height, uint8_t *src_nv12_data);

    // 镜像
    static void mirrorI420(uint8_t *src_i420_data, int width, int height, uint8_t *dst_i420_data);

    // 旋转
    static void rotateI420(uint8_t *src_i420_data, int width, int height, uint8_t *dst_i420_data, int degree);

    // 缩放
    static void scaleI420(uint8_t *src_i420_data, int width, int height, uint8_t *dst_i420_data, int dst_width,
                          int dst_height, int mode);

    // 裁剪
    static void cropI420(uint8_t *src_i420_data, int src_length, int width, int height,
                         uint8_t *dst_i420_data, int dst_width, int dst_height, int left, int top);
};

#endif //EPLAYER_YUVUTIL_H
