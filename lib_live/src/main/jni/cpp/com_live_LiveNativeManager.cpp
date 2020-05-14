//
// Created by public on 2019/9/18.
//
#include <jni.h>
#include <string>
#include <libyuv.h>
#include <libyuv/rotate.h>
#include "header/AndroidLog.h"
#include "header/VideoEncoder.h"
#include "header/AudioEncoder.h"
#include "header/RtmpLivePush.h"

jbyte *temp_i420_data;
jbyte *temp_i420_data_scale;
jbyte *temp_i420_data_rotate;

VideoEncoder *videoEncoder;
AudioEncoder *audioEncoder;
RtmpLivePush *rtmpLivePush;

// 声明几个回调函数
extern void audioEncodeCallback(uint8_t *data ,int dataLen);

static JavaVM *jvm = NULL;

//在库加载时执行
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    jvm = vm;
    return JNI_VERSION_1_6;
}

// 在卸载库时执行
void JNI_OnUnload(JavaVM *vm, void *reserved) {
    jvm = NULL;
}


// 为中间操作的需要分配空间
void init(jint width, jint height, jint dst_width, jint dst_height) {
    // i420临时原始数据的缓存
    temp_i420_data = static_cast<jbyte *>(malloc(sizeof(jbyte) * width * height * 3 / 2));
    // i420临时压缩数据的缓存
    temp_i420_data_scale = static_cast<jbyte *>(malloc(sizeof(jbyte) * dst_width * dst_height * 3 / 2));
    temp_i420_data_rotate = static_cast<jbyte *>(malloc(sizeof(jbyte) * dst_width * dst_height * 3 / 2));

}

/**
 * 对源数据进行缩减操作，比如将1080*1920的YUV420p的数据缩小为480*640的YUV420p数据
 * @param src_i420_data
 * @param width
 * @param height
 * @param dst_i420_data
 * @param dst_width
 * @param dst_height
 * @param mode
 */
void scaleI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data, jint dst_width, jint dst_height,
               jint mode) {
    //Y数据大小width*height，U数据大小为1/4的width*height，V大小和U一样，一共是3/2的width*height大小
    jint src_i420_y_size = width * height;
    jint src_i420_u_size = (width >> 1) * (height >> 1);
    //由于是标准的YUV420P的数据，我们可以把三个通道全部分离出来
    jbyte *src_i420_y_data = src_i420_data;
    jbyte *src_i420_u_data = src_i420_data + src_i420_y_size;
    jbyte *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    //由于是标准的YUV420P的数据，我们可以把三个通道全部分离出来
    jint dst_i420_y_size = dst_width * dst_height;
    jint dst_i420_u_size = (dst_width >> 1) * (dst_height >> 1);
    jbyte *dst_i420_y_data = dst_i420_data;
    jbyte *dst_i420_u_data = dst_i420_data + dst_i420_y_size;
    jbyte *dst_i420_v_data = dst_i420_data + dst_i420_y_size + dst_i420_u_size;

    //调用libyuv库，进行缩放操作，因为src是不变的，可以用const修饰，而dst是可变的，所以不用const修饰
    libyuv::I420Scale((const uint8 *) src_i420_y_data, width,
                      (const uint8 *) src_i420_u_data, width >> 1,
                      (const uint8 *) src_i420_v_data, width >> 1,
                      width, height,
                      (uint8 *) dst_i420_y_data, dst_width,
                      (uint8 *) dst_i420_u_data, dst_width >> 1,
                      (uint8 *) dst_i420_v_data, dst_width >> 1,
                      dst_width, dst_height,
                      (libyuv::FilterMode) mode);
}

//旋转以后width和height是相反的
void rotateI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data, jint degree) {
    jint src_i420_y_size = width * height;
    jint src_i420_u_size = (width >> 1) * (height >> 1);

    jbyte *src_i420_y_data = src_i420_data;
    jbyte *src_i420_u_data = src_i420_data + src_i420_y_size;
    jbyte *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    jbyte *dst_i420_y_data = dst_i420_data;
    jbyte *dst_i420_u_data = dst_i420_data + src_i420_y_size;
    jbyte *dst_i420_v_data = dst_i420_data + src_i420_y_size + src_i420_u_size;
    //LOGE("角度=%d",degree);
    //要注意这里的width和height在旋转之后是相反的
    if (degree == libyuv::kRotate90 || degree == libyuv::kRotate270) {
        //原来的行间隔为width，旋转以后行间隔变为height，就是宽高交换了
        libyuv::I420Rotate((const uint8 *) src_i420_y_data, width,
                           (const uint8 *) src_i420_u_data, width >> 1,
                           (const uint8 *) src_i420_v_data, width >> 1,
                           (uint8 *) dst_i420_y_data, height,
                           (uint8 *) dst_i420_u_data, height >> 1,
                           (uint8 *) dst_i420_v_data, height >> 1,
                           width, height,
                           (libyuv::RotationMode) degree);
    } else {
        libyuv::I420Rotate((const uint8 *) src_i420_y_data, width,
                           (const uint8 *) src_i420_u_data, width >> 1,
                           (const uint8 *) src_i420_v_data, width >> 1,
                           (uint8 *) dst_i420_y_data, width,
                           (uint8 *) dst_i420_u_data, width >> 1,
                           (uint8 *) dst_i420_v_data, width >> 1,
                           width, height,
                           (libyuv::RotationMode) degree);

    }
}

void mirrorI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data) {
    jint src_i420_y_size = width * height;
    jint src_i420_u_size = (width >> 1) * (height >> 1);

    jbyte *src_i420_y_data = src_i420_data;
    jbyte *src_i420_u_data = src_i420_data + src_i420_y_size;
    jbyte *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    jbyte *dst_i420_y_data = dst_i420_data;
    jbyte *dst_i420_u_data = dst_i420_data + src_i420_y_size;
    jbyte *dst_i420_v_data = dst_i420_data + src_i420_y_size + src_i420_u_size;

    libyuv::I420Mirror((const uint8 *) src_i420_y_data, width,
                       (const uint8 *) src_i420_u_data, width >> 1,
                       (const uint8 *) src_i420_v_data, width >> 1,
                       (uint8 *) dst_i420_y_data, width,
                       (uint8 *) dst_i420_u_data, width >> 1,
                       (uint8 *) dst_i420_v_data, width >> 1,
                       width, height);
}

//NV21转化为YUV420P数据
void nv21ToI420(jbyte *src_nv21_data, jint width, jint height, jbyte *src_i420_data) {
    //Y通道数据大小
    jint src_y_size = width * height;
    //U通道数据大小
    jint src_u_size = (width >> 1) * (height >> 1);

    //NV21中Y通道数据，定义一个byte指针指向NV21数据中的起点，即指向了Y通道数据的起点
    jbyte *src_nv21_y_data = src_nv21_data;
    //定义的指针指向VU数据的起点，即在所有数据的起点+Y数据的大小就是VU数据的起点，指针都是指向保存数据内存中的第一个内存地址
    //由于是连续存储的Y通道数据后即为VU数据，它们的存储方式是交叉存储的
    jbyte *src_nv21_vu_data = src_nv21_data + src_y_size;

    //YUV420P中Y通道数据
    jbyte *src_i420_y_data = src_i420_data;
    //YUV420P中U通道数据
    jbyte *src_i420_u_data = src_i420_data + src_y_size;
    //YUV420P中V通道数据
    jbyte *src_i420_v_data = src_i420_data + src_y_size + src_u_size;

    //直接调用libyuv中接口，把NV21数据转化为YUV420P标准数据，此时，它们的存储大小是不变的
    //参数src_stride_y和src_stride_vu表示Y和VU的行间距，都应该传递源Y平面的宽
    libyuv::NV21ToI420((const uint8 *) src_nv21_y_data, width,
                       (const uint8 *) src_nv21_vu_data, width,
                       (uint8 *) src_i420_y_data, width,
                       (uint8 *) src_i420_u_data, width >> 1,
                       (uint8 *) src_i420_v_data, width >> 1,
                       width, height);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_release(JNIEnv *env, jclass type) {
    LOGD("调用了LiveNativeManager_release");
    free(temp_i420_data);
    free(temp_i420_data_scale);
    free(temp_i420_data_rotate);
    delete (videoEncoder);
    delete (audioEncoder);
    delete (rtmpLivePush);
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_yuvI420ToNV21(JNIEnv *env, jclass type, jbyteArray i420Src_, jbyteArray nv21Src_,
                                              jint width, jint height) {

    jbyte *i420Src = env->GetByteArrayElements(i420Src_, NULL);
    jbyte *nv21Src = env->GetByteArrayElements(nv21Src_, NULL);

    jint src_y_size = width * height;
    jint src_u_size = (width >> 1) * (height >> 1);

    jbyte *src_i420_y_data = i420Src;
    jbyte *src_i420_u_data = i420Src + src_y_size;
    jbyte *src_i420_v_data = i420Src + src_y_size + src_u_size;

    jbyte *src_nv21_y_data = nv21Src;
    jbyte *src_nv21_vu_data = nv21Src + src_y_size;


    libyuv::I420ToNV21(
            (const uint8 *) src_i420_y_data, width,
            (const uint8 *) src_i420_u_data, width >> 1,
            (const uint8 *) src_i420_v_data, width >> 1,
            (uint8 *) src_nv21_y_data, width,
            (uint8 *) src_nv21_vu_data, width,
            width, height);

    env->ReleaseByteArrayElements(i420Src_, i420Src, 0);
    env->ReleaseByteArrayElements(nv21Src_, nv21Src, 0);

    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_compressYUV(JNIEnv *env, jclass type, jbyteArray src_, jint width, jint height,
                                            jbyteArray dst_, jint dst_width, jint dst_height, jint mode,
                                            jint degree,
                                            jboolean isMirror) {

    jbyte *src = env->GetByteArrayElements(src_, NULL);
    jbyte *dst = env->GetByteArrayElements(dst_, NULL);

    // nv21转化为i420(标准YUV420P数据) 这个temp_i420_data大小是和Src_data是一样的，是原始YUV数据的大小
    nv21ToI420(src, width, height, temp_i420_data);
    // 进行缩放的操作，这个缩放，会把数据压缩
    scaleI420(temp_i420_data, width, height, temp_i420_data_scale, dst_width, dst_height, mode);

    // 如果是前置摄像头，进行镜像操作
    if (isMirror) {
        // 进行旋转的操作
        rotateI420(temp_i420_data_scale, dst_width, dst_height, temp_i420_data_rotate, degree);
        // 因为旋转的角度都是90和270，那后面的数据width和height是相反的
        mirrorI420(temp_i420_data_rotate, dst_height, dst_width, dst);
    } else {
        // 进行旋转的操作
        rotateI420(temp_i420_data_scale, dst_width, dst_height, dst, degree);
    }

    env->ReleaseByteArrayElements(src_, src, 0);
    env->ReleaseByteArrayElements(dst_, dst, 0);

    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_cropYUV(JNIEnv *env, jclass type, jbyteArray src_, jint width, jint height,
                                        jbyteArray dst_, jint dst_width, jint dst_height, jint left, jint top) {

    //裁剪的区域大小不对，left指宽度从哪里开始剪裁，dst_width指从剪裁处开始算的目标宽度，top指高度从哪里开始剪裁
    if (left + dst_width > width || top + dst_height > height) {
        return -1;
    }

    //left和top必须为偶数，否则显示会有问题
    if (left % 2 != 0 || top % 2 != 0) {
        return -1;
    }
    jint src_length = env->GetArrayLength(src_); //源数据大小
    jbyte *src = env->GetByteArrayElements(src_, NULL);
    jbyte *dst = env->GetByteArrayElements(dst_, NULL);

    jint dst_i420_y_size = dst_width * dst_height;
    jint dst_i420_u_size = (dst_width >> 1) * (dst_height >> 1);

    jbyte *dst_i420_y_data = dst;
    jbyte *dst_i420_u_data = dst + dst_i420_y_size;
    jbyte *dst_i420_v_data = dst + dst_i420_y_size + dst_i420_u_size;

    libyuv::ConvertToI420((const uint8 *) src, src_length,
                          (uint8 *) dst_i420_y_data, dst_width,
                          (uint8 *) dst_i420_u_data, dst_width >> 1,
                          (uint8 *) dst_i420_v_data, dst_width >> 1,
                          left, top,
                          width, height,
                          dst_width, dst_height,
                          libyuv::kRotate0, libyuv::FOURCC_I420);

    env->ReleaseByteArrayElements(src_, src, 0);
    env->ReleaseByteArrayElements(dst_, dst, 0);
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_encoderVideoinit(JNIEnv *env, jclass type, jint src_width,
                                                 jint src_height,jint in_width, jint in_height) {
    // 初始化临时空间
    init(src_width,src_height,in_width,in_height);

    videoEncoder = new VideoEncoder();
    //设置相关参数
    videoEncoder->setInWidth(in_width);
    videoEncoder->setInHeight(in_height);
    videoEncoder->setBitrate(1200 * 1000); //设置比特率
    videoEncoder->open();
    return 0;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_encoderVideoEncode(JNIEnv *env, jclass type, jbyteArray srcFrame_, jint frameSize,
                                                   jint fps, jbyteArray dstFrame_, jintArray outFramewSize_) {
    // 转为jbyte数组的指针
    jbyte *srcFrame = env->GetByteArrayElements(srcFrame_, NULL);
    jbyte *dstFrame = env->GetByteArrayElements(dstFrame_, NULL);
    jint *outFramewSize = env->GetIntArrayElements(outFramewSize_, NULL);

    int numNals = videoEncoder->encodeFrame((uint8_t *) srcFrame, frameSize, fps, (char *) dstFrame, outFramewSize);

    env->ReleaseByteArrayElements(srcFrame_, srcFrame, 0);
    env->ReleaseByteArrayElements(dstFrame_, dstFrame, 0);
    env->ReleaseIntArrayElements(outFramewSize_, outFramewSize, 0);

    return numNals;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_encoderAudioInit(JNIEnv *env, jclass type, jint sampleRate, jint channels,
                                                 jint bitRate) {
    audioEncoder = new AudioEncoder(channels, sampleRate, bitRate);
    int value = audioEncoder->init();
    audioEncoder->setEncodeCallback(&audioEncodeCallback);
    return value;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_encoderAudioEncode(JNIEnv *env, jclass type, jbyteArray srcFrame_, jint frameSize,
                                                   jbyteArray dstFrame_, jint dstSize) {
    jbyte *srcFrame = env->GetByteArrayElements(srcFrame_, NULL);
    jbyte *dstFrame = env->GetByteArrayElements(dstFrame_, NULL);

    int validlength = audioEncoder->encodeAudio((unsigned char *) srcFrame, frameSize,
                                                (unsigned char *) dstFrame, dstSize);

    env->ReleaseByteArrayElements(srcFrame_, srcFrame, 0);
    env->ReleaseByteArrayElements(dstFrame_, dstFrame, 0);

    return validlength;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_initRtmpData(JNIEnv *env, jclass type, jstring url_) {

    const char *url = env->GetStringUTFChars(url_, 0);

    //复制url内容到rtmp_path
    char *rtmp_path = (char *) malloc(strlen(url) + 1);
    memset(rtmp_path, 0, strlen(url) + 1);
    memcpy(rtmp_path, url, strlen(url));

    rtmpLivePush = new RtmpLivePush();
    rtmpLivePush->init((unsigned char *) rtmp_path);

    env->ReleaseStringUTFChars(url_, url);

    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_sendRtmpVideoSpsPPS(JNIEnv *env, jclass type, jbyteArray sps_, jint spsLen,
                                                    jbyteArray pps_, jint ppsLen, jlong timeStamp) {
    if (rtmpLivePush) {
        jbyte *sps = env->GetByteArrayElements(sps_, NULL);
        jbyte *pps = env->GetByteArrayElements(pps_, NULL);

        rtmpLivePush->pushSPSPPS((unsigned char *) sps, spsLen, (unsigned char *) pps, ppsLen);
        //rtmpLivePush->send_video_sps_pps((unsigned char *) sps, spsLen,(unsigned char *) pps, ppsLen);

        env->ReleaseByteArrayElements(sps_, sps, 0);
        env->ReleaseByteArrayElements(pps_, pps, 0);
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_sendRtmpVideoData(JNIEnv *env, jclass type, jbyteArray data_, jint dataLen,
                                                  jlong timeStamp) {
    if (rtmpLivePush) {
        jbyte *data = env->GetByteArrayElements(data_, NULL);

        rtmpLivePush->addH264Body((unsigned char *) data, dataLen, timeStamp);

        env->ReleaseByteArrayElements(data_, data, 0);
    }
    return 0;
}

//发送音频Sequence头数据
extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_sendRtmpAudioSpec(JNIEnv *env, jclass type, jlong timeStamp) {

    if (rtmpLivePush) {
        //直接指定为44100采样率，2声道
        rtmpLivePush->addSequenceAacHeader(44100, 2, 0);
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_sendRtmpAudioData(JNIEnv *env, jclass type, jbyteArray data_, jint dataLen,
                                                  jlong timeStamp) {

    if (rtmpLivePush) {
        jbyte *data = env->GetByteArrayElements(data_, NULL);
        //c++ 中没有byte类型，用unsigned char代替
        rtmpLivePush->addAccBody((unsigned char *) data, dataLen, timeStamp);

        env->ReleaseByteArrayElements(data_, data, 0);
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_releaseRtmp(JNIEnv *env, jclass type) {

    if (rtmpLivePush) {
        delete (rtmpLivePush);
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_releaseVideo(JNIEnv *env, jclass type) {

    if (videoEncoder) {
        delete (videoEncoder);
        free(temp_i420_data);
        free(temp_i420_data_scale);
        free(temp_i420_data_rotate);
    }
    return 0;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_releaseAudio(JNIEnv *env, jclass type) {

    if (audioEncoder) {
        delete (audioEncoder);
    }
    return 0;
}

/**
 * 音频编码结果的回调函数
 * @param data
 * @param dataLen
 */
void audioEncodeCallback(uint8_t *data ,int dataLen){
    LOGD("音频回调函数");
}



