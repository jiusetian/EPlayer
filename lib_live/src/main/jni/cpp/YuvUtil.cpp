//
// Created by lxr on 2019/10/16.
//
#include "header/YuvUtil.h"
#include "header/AndroidLog.h"
#include <libyuv.h>

// nv21 --> i420
void YuvUtil::nv21ToI420(uint8_t *src_nv21_data, int width, int height, uint8_t *src_i420_data) {
    int src_y_size = width * height;
    int src_u_size = (width >> 1) * (height >> 1);

    uint8_t *src_nv21_y_data = src_nv21_data;
    uint8_t *src_nv21_vu_data = src_nv21_data + src_y_size;

    // 转换后的YUV数据
    uint8_t *src_i420_y_data = src_i420_data;
    uint8_t *src_i420_u_data = src_i420_data + src_y_size;
    uint8_t *src_i420_v_data = src_i420_data + src_y_size + src_u_size;

    libyuv::NV21ToI420((const uint8 *) src_nv21_y_data, width,
                       (const uint8 *) src_nv21_vu_data, width,
                       src_i420_y_data, width,
                       src_i420_u_data, width >> 1,
                       src_i420_v_data, width >> 1,
                       width, height);
}

// i420 --> nv21
void YuvUtil::i420ToNv21(uint8_t *src_i420_data, int width, int height, uint8_t *src_nv21_data) {
    int src_y_size = width * height;
    int src_u_size = (width >> 1) * (height >> 1);

    uint8_t *src_nv21_y_data = src_nv21_data;
    uint8_t *src_nv21_uv_data = src_nv21_data + src_y_size;

    uint8_t *src_i420_y_data = src_i420_data;
    uint8_t *src_i420_u_data = src_i420_data + src_y_size;
    uint8_t *src_i420_v_data = src_i420_data + src_y_size + src_u_size;


    libyuv::I420ToNV21(
            (const uint8 *) src_i420_y_data, width,
            (const uint8 *) src_i420_u_data, width >> 1,
            (const uint8 *) src_i420_v_data, width >> 1,
            src_nv21_y_data, width,
            src_nv21_uv_data, width,
            width, height);
}

// nv12 --> i420
void YuvUtil::nv12ToI420(uint8_t *Src_data, int src_width, int src_height, uint8_t *Dst_data) {
    // NV12 video size
    int NV12_Size = src_width * src_height * 3 / 2;
    int NV12_Y_Size = src_width * src_height;

    // YUV420 video size
    int I420_Size = src_width * src_height * 3 / 2;
    int I420_Y_Size = src_width * src_height;
    int I420_U_Size = (src_width >> 1) * (src_height >> 1);
    int I420_V_Size = I420_U_Size;

    // src: buffer address of Y channel and UV channel
    uint8_t *Y_data_Src = Src_data;
    uint8_t *UV_data_Src = Src_data + NV12_Y_Size;
    int src_stride_y = src_width;
    int src_stride_uv = src_width;

    //dst: buffer address of Y channel、U channel and V channel
    uint8_t *Y_data_Dst = Dst_data;
    uint8_t *U_data_Dst = Dst_data + I420_Y_Size;
    uint8_t *V_data_Dst = Dst_data + I420_Y_Size + I420_U_Size;
    int Dst_Stride_Y = src_width;
    int Dst_Stride_U = src_width >> 1;
    int Dst_Stride_V = Dst_Stride_U;

    libyuv::NV12ToI420((const uint8 *) Y_data_Src, src_stride_y,
                       (const uint8 *) UV_data_Src, src_stride_uv,
                       Y_data_Dst, Dst_Stride_Y,
                       U_data_Dst, Dst_Stride_U,
                       V_data_Dst, Dst_Stride_V,
                       src_width, src_height);
}

// i420 --> nv12
void YuvUtil::i420ToNv12(uint8_t *src_i420_data, int width, int height, uint8_t *src_nv12_data) {
    int src_y_size = width * height;
    int src_u_size = (width >> 1) * (height >> 1);

    uint8_t *src_nv12_y_data = src_nv12_data;
    uint8_t *src_nv12_uv_data = src_nv12_data + src_y_size;

    uint8_t *src_i420_y_data = src_i420_data;
    uint8_t *src_i420_u_data = src_i420_data + src_y_size;
    uint8_t *src_i420_v_data = src_i420_data + src_y_size + src_u_size;

    libyuv::I420ToNV12(
            (const uint8 *) src_i420_y_data, width,
            (const uint8 *) src_i420_u_data, width >> 1,
            (const uint8 *) src_i420_v_data, width >> 1,
            src_nv12_y_data, width,
            src_nv12_uv_data, width,
            width, height);
}

// 镜像
void YuvUtil::mirrorI420(uint8_t *src_i420_data, int width, int height, uint8_t *dst_i420_data) {
    int src_i420_y_size = width * height;
    // int src_i420_u_size = (width >> 1) * (height >> 1);
    int src_i420_u_size = src_i420_y_size >> 2;

    uint8_t *src_i420_y_data = src_i420_data;
    uint8_t *src_i420_u_data = src_i420_data + src_i420_y_size;
    uint8_t *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    uint8_t *dst_i420_y_data = dst_i420_data;
    uint8_t *dst_i420_u_data = dst_i420_data + src_i420_y_size;
    uint8_t *dst_i420_v_data = dst_i420_data + src_i420_y_size + src_i420_u_size;

    libyuv::I420Mirror((const uint8 *) src_i420_y_data, width,
                       (const uint8 *) src_i420_u_data, width >> 1,
                       (const uint8 *) src_i420_v_data, width >> 1,
                       dst_i420_y_data, width,
                       dst_i420_u_data, width >> 1,
                       dst_i420_v_data, width >> 1,
                       width, height);
}

// 旋转
void YuvUtil::rotateI420(uint8_t *src_i420_data, int width, int height, uint8_t *dst_i420_data, int degree) {
    int src_i420_y_size = width * height;
    int src_i420_u_size = (width >> 1) * (height >> 1);

    uint8_t *src_i420_y_data = src_i420_data;
    uint8_t *src_i420_u_data = src_i420_data + src_i420_y_size;
    uint8_t *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    uint8_t *dst_i420_y_data = dst_i420_data;
    uint8_t *dst_i420_u_data = dst_i420_data + src_i420_y_size;
    uint8_t *dst_i420_v_data = dst_i420_data + src_i420_y_size + src_i420_u_size;

    //要注意这里的width和height在旋转之后是相反的
    if (degree == libyuv::kRotate90 || degree == libyuv::kRotate270) {
        libyuv::I420Rotate((const uint8 *) src_i420_y_data, width,
                           (const uint8 *) src_i420_u_data, width >> 1,
                           (const uint8 *) src_i420_v_data, width >> 1,
                           dst_i420_y_data, height,
                           dst_i420_u_data, height >> 1,
                           dst_i420_v_data, height >> 1,
                           width, height,
                           (libyuv::RotationMode) degree);
    } else {
        libyuv::I420Rotate((const uint8 *) src_i420_y_data, width,
                           (const uint8 *) src_i420_u_data, width >> 1,
                           (const uint8 *) src_i420_v_data, width >> 1,
                           dst_i420_y_data, width,
                           dst_i420_u_data, width >> 1,
                           dst_i420_v_data, width >> 1,
                           width, height,
                           (libyuv::RotationMode) degree);
    }
}

// 缩放
void YuvUtil::scaleI420(uint8_t *src_i420_data, int width, int height, uint8_t *dst_i420_data, int dst_width,
                        int dst_height, int mode) {

    int src_i420_y_size = width * height;
    int src_i420_u_size = (width >> 1) * (height >> 1);
    uint8_t *src_i420_y_data = src_i420_data;
    uint8_t *src_i420_u_data = src_i420_data + src_i420_y_size;
    uint8_t *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    int dst_i420_y_size = dst_width * dst_height;
    int dst_i420_u_size = (dst_width >> 1) * (dst_height >> 1);
    uint8_t *dst_i420_y_data = dst_i420_data;
    uint8_t *dst_i420_u_data = dst_i420_data + dst_i420_y_size;
    uint8_t *dst_i420_v_data = dst_i420_data + dst_i420_y_size + dst_i420_u_size;

    libyuv::I420Scale((const uint8 *) src_i420_y_data, width,
                      (const uint8 *) src_i420_u_data, width >> 1,
                      (const uint8 *) src_i420_v_data, width >> 1,
                      width, height,
                      dst_i420_y_data, dst_width,
                      dst_i420_u_data, dst_width >> 1,
                      dst_i420_v_data, dst_width >> 1,
                      dst_width, dst_height,
                      (libyuv::FilterMode) mode);
}

// 裁剪
void YuvUtil::cropI420(uint8_t *src_i420_data, int src_length, int width, int height,
                       uint8_t *dst_i420_data, int dst_width, int dst_height, int left, int top) {

    //裁剪的区域大小不对，left指宽度从哪里开始剪裁，dst_width指从剪裁处开始算的目标宽度，top指高度从哪里开始剪裁
    if (left + dst_width > width || top + dst_height > height) {
        return;
    }
    //left和top必须为偶数，否则显示会有问题
    if (left % 2 != 0 || top % 2 != 0) {
        return;
    }
    int dst_i420_y_size = dst_width * dst_height;
    int dst_i420_u_size = (dst_width >> 1) * (dst_height >> 1);

    uint8_t *dst_i420_y_data = dst_i420_data;
    uint8_t *dst_i420_u_data = dst_i420_data + dst_i420_y_size;
    uint8_t *dst_i420_v_data = dst_i420_data + dst_i420_y_size + dst_i420_u_size;

    libyuv::ConvertToI420((const uint8 *) src_i420_data, src_length,
                          dst_i420_y_data, dst_width,
                          dst_i420_u_data, dst_width >> 1,
                          dst_i420_v_data, dst_width >> 1,
                          left, top,
                          width, height,
                          dst_width, dst_height,
                          libyuv::kRotate0, libyuv::FOURCC_I420);
}

