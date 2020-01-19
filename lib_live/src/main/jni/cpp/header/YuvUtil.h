//
// Created by lxr on 2019/10/16.
//

#ifndef EPLAYER_YUVUTIL_H
#define EPLAYER_YUVUTIL_H

#include <jni.h>

class YuvUtil {
public:
    // nv21 --> i420
    void nv21ToI420(jbyte *src_nv21_data, jint width, jint height, jbyte *src_i420_data);

    // i420 --> nv21
    void i420ToNv21(jbyte *src_i420_data, jint width, jint height, jbyte *src_nv21_data);

    // nv12 --> i420
    void nv12ToI420(jbyte *Src_data, jint src_width, jint src_height, jbyte *Dst_data);

    // i420 --> nv12
    void i420ToNv12(jbyte *src_i420_data, jint width, jint height, jbyte *src_nv12_data);

    // 镜像
    void mirrorI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data);

    // 旋转
    void rotateI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data, jint degree);

    // 缩放
    void scaleI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data, jint dst_width,
                   jint dst_height, jint mode);

    // 裁剪
    void cropI420(jbyte *src_i420_data, jint src_length, jint width, jint height,
                  jbyte *dst_i420_data, jint dst_width, jint dst_height, jint left, jint top);
};

#endif //EPLAYER_YUVUTIL_H
