// 
//  Created by public on 2019/9/18.
// 
#include <jni.h>
#include <string>
#include <libyuv.h>
#include <libyuv/rotate.h>
#include "header/AndroidLog.h"
#include "header/VideoEncoder.h"
#include "header/AudioEncoder.h"
#include "header/RtmpLivePush.h"
#include "YuvProcess.h"

jbyte *temp_i420_data;
jbyte *temp_i420_data_scale;
jbyte *temp_i420_data_rotate;
jint mOriantation = 0;

YuvProcess *yuvProcess = nullptr;
VideoEncoder *videoEncoder = nullptr;
AudioEncoder *audioEncoder = nullptr;
RtmpLivePush *rtmpLivePush = nullptr;

//  声明几个回调函数
extern void audioEncoderCallback(uint8_t *data, int dataLen);

extern void yuvProcessCallback(uint8_t *data, int dataLen);

extern void videoEncoderCallback(uint8_t *data, int dataLen, int nalNum, int *nalsSize);

static JavaVM *jvm = NULL;

// 在库加载时执行
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    jvm = vm;
    return JNI_VERSION_1_6;
}

//  在卸载库时执行
void JNI_OnUnload(JavaVM *vm, void *reserved) {
    jvm = NULL;
}


//  为中间操作的需要分配空间
void init(jint width, jint height, jint dst_width, jint dst_height) {
    //  i420临时原始数据的缓存
    temp_i420_data = static_cast<jbyte *>(malloc(sizeof(jbyte) * width * height * 3 / 2));
    //  i420临时压缩数据的缓存
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
    // Y数据大小width*height，U数据大小为1/4的width*height，V大小和U一样，一共是3/2的width*height大小
    jint src_i420_y_size = width * height;
    jint src_i420_u_size = (width >> 1) * (height >> 1);
    // 由于是标准的YUV420P的数据，我们可以把三个通道全部分离出来
    jbyte *src_i420_y_data = src_i420_data;
    jbyte *src_i420_u_data = src_i420_data + src_i420_y_size;
    jbyte *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    // 由于是标准的YUV420P的数据，我们可以把三个通道全部分离出来
    jint dst_i420_y_size = dst_width * dst_height;
    jint dst_i420_u_size = (dst_width >> 1) * (dst_height >> 1);
    jbyte *dst_i420_y_data = dst_i420_data;
    jbyte *dst_i420_u_data = dst_i420_data + dst_i420_y_size;
    jbyte *dst_i420_v_data = dst_i420_data + dst_i420_y_size + dst_i420_u_size;

    // 调用libyuv库，进行缩放操作，因为src是不变的，可以用const修饰，而dst是可变的，所以不用const修饰
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

void rotateI420(jbyte *src_i420_data, jint width, jint height, jbyte *dst_i420_data, jint degree) {
    jint src_i420_y_size = width * height;
    jint src_i420_u_size = (width >> 1) * (height >> 1);

    jbyte *src_i420_y_data = src_i420_data;
    jbyte *src_i420_u_data = src_i420_data + src_i420_y_size;
    jbyte *src_i420_v_data = src_i420_data + src_i420_y_size + src_i420_u_size;

    jbyte *dst_i420_y_data = dst_i420_data;
    jbyte *dst_i420_u_data = dst_i420_data + src_i420_y_size;
    jbyte *dst_i420_v_data = dst_i420_data + src_i420_y_size + src_i420_u_size;
    // LOGE("角度=%d",degree);
    // 要注意这里的width和height在旋转之后是相反的
    if (degree == libyuv::kRotate90 || degree == libyuv::kRotate270) {  // 竖屏
        // 因为旋转90或270度以后，宽高跟原来的正好相反，所以旋转后宽就是原来的高了
        libyuv::I420Rotate((const uint8 *) src_i420_y_data, width,
                           (const uint8 *) src_i420_u_data, width >> 1,
                           (const uint8 *) src_i420_v_data, width >> 1,
                           (uint8 *) dst_i420_y_data, height,
                           (uint8 *) dst_i420_u_data, height >> 1,
                           (uint8 *) dst_i420_v_data, height >> 1,
                           width, height,
                           (libyuv::RotationMode) degree);
    } else { // 横屏
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

// NV21转化为YUV420P数据
void nv21ToI420(jbyte *src_nv21_data, jint width, jint height, jbyte *src_i420_data) {
    // Y通道数据大小
    jint src_y_size = width * height;
    // U通道数据大小
    jint src_u_size = (width >> 1) * (height >> 1);

    // NV21中Y通道数据，定义一个byte指针指向NV21数据中的起点，即指向了Y通道数据的起点
    jbyte *src_nv21_y_data = src_nv21_data;
    // 定义的指针指向VU数据的起点，即在所有数据的起点+Y数据的大小就是VU数据的起点，指针都是指向保存数据内存中的第一个内存地址
    // 由于是连续存储的Y通道数据后即为VU数据，它们的存储方式是交叉存储的
    jbyte *src_nv21_vu_data = src_nv21_data + src_y_size;

    // YUV420P中Y通道数据
    jbyte *src_i420_y_data = src_i420_data;
    // YUV420P中U通道数据
    jbyte *src_i420_u_data = src_i420_data + src_y_size;
    // YUV420P中V通道数据
    jbyte *src_i420_v_data = src_i420_data + src_y_size + src_u_size;

    // 直接调用libyuv中接口，把NV21数据转化为YUV420P标准数据，此时，它们的存储大小是不变的
    // 参数src_stride_y和src_stride_vu表示Y和VU的行间距，都应该传递源Y平面的宽
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
    if (videoEncoder)
        delete (videoEncoder);
    if (audioEncoder)
        delete (audioEncoder);
    if (rtmpLivePush)
        delete (rtmpLivePush);
    if (yuvProcess)
        delete (yuvProcess);
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

    //  nv21转化为i420(标准YUV420P数据) 这个temp_i420_data大小是和Src_data是一样的，是原始YUV数据的大小
    nv21ToI420(src, width, height, temp_i420_data);
    //  进行缩放的操作，这个缩放，会把数据压缩
    scaleI420(temp_i420_data, width, height, temp_i420_data_scale, dst_width, dst_height, mode);
    // 视频角度发生改变
    if (degree != mOriantation) {
        mOriantation = degree;
        // 竖屏宽高要交换，因为手机摄像头的视频输出总是宽大于高的，而实际上竖屏的图像显示是高大于宽的
        if (degree == 90 || degree == 270) {
            videoEncoder->setInWidth(dst_height);
            videoEncoder->setInHeight(dst_width);
        } else {
            videoEncoder->setInWidth(dst_width);
            videoEncoder->setInHeight(dst_height);
        }
    }

    //  如果是前置摄像头，进行镜像操作
    if (isMirror) {
        //  进行旋转的操作
        rotateI420(temp_i420_data_scale, dst_width, dst_height, temp_i420_data_rotate, degree);
        //  因为270度才需要镜像的，此时是需要旋转270度的，旋转以后宽高就交换了
        mirrorI420(temp_i420_data_rotate, dst_height, dst_width, dst);
    } else {
        //  进行旋转的操作
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

    // 裁剪的区域大小不对，left指宽度从哪里开始剪裁，dst_width指从剪裁处开始算的目标宽度，top指高度从哪里开始剪裁
    if (left + dst_width > width || top + dst_height > height) {
        return -1;
    }

    // left和top必须为偶数，否则显示会有问题
    if (left % 2 != 0 || top % 2 != 0) {
        return -1;
    }
    jint src_length = env->GetArrayLength(src_); // 源数据大小
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
Java_com_live_LiveNativeManager_videoEncoderinit(JNIEnv *env, jclass type, jint src_width,
                                                 jint src_height, jint in_width, jint in_height, jint orientation) {
    //  初始化临时空间
    init(src_width, src_height, in_width, in_height);
    mOriantation = orientation;
    videoEncoder = new VideoEncoder();
    // 设置相关参数
    if (orientation == 90 || orientation == 270) { // 竖屏，输出宽高要交换
        videoEncoder->setInWidth(in_height);
        videoEncoder->setInHeight(in_width);
    } else { // 横屏
        videoEncoder->setInWidth(in_width);
        videoEncoder->setInHeight(in_height);
    }
    videoEncoder->setBitrate(1200 * 1000); // 设置比特率
    videoEncoder->open();
    return 0;

}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_videoEncode(JNIEnv *env, jclass type, jbyteArray srcFrame_, jint frameSize,
                                            jint fps, jbyteArray dstFrame_, jintArray outFramewSize_) {
    //  转为jbyte数组的指针
    jbyte *srcFrame = env->GetByteArrayElements(srcFrame_, NULL);
    jbyte *dstFrame = env->GetByteArrayElements(dstFrame_, NULL);
    jint *outFramewSize = env->GetIntArrayElements(outFramewSize_, NULL);

    int numNals = videoEncoder->encodeFrame((uint8_t *) srcFrame, frameSize, fps, (unsigned char *) dstFrame,
                                            outFramewSize);

    env->ReleaseByteArrayElements(srcFrame_, srcFrame, 0);
    env->ReleaseByteArrayElements(dstFrame_, dstFrame, 0);
    env->ReleaseIntArrayElements(outFramewSize_, outFramewSize, 0);

    return numNals;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_initAudioEncoder(JNIEnv *env, jclass type, jint sampleRate, jint channels,
                                                 jint bitRate) {
    audioEncoder = new AudioEncoder(channels, sampleRate, bitRate);
    int value = audioEncoder->init();
    //  set encoder callback for result
    audioEncoder->setEncoderCallback(&audioEncoderCallback);
    return value;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_audioEncode(JNIEnv *env, jclass type, jbyteArray srcFrame_, jint frameSize,
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

    // 复制url内容到rtmp_path
    char *rtmp_path = (char *) malloc(strlen(url) + 1);
    memset(rtmp_path, 0, strlen(url) + 1);
    memcpy(rtmp_path, url, strlen(url));

    rtmpLivePush = new RtmpLivePush();
    rtmpLivePush->init((unsigned char *) rtmp_path);
    free(rtmp_path);
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
        // rtmpLivePush->send_video_sps_pps((unsigned char *) sps, spsLen,(unsigned char *) pps, ppsLen);

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

        rtmpLivePush->pushH264Body((unsigned char *) data, dataLen, timeStamp);

        env->ReleaseByteArrayElements(data_, data, 0);
    }
    return 0;
}

// 发送音频Sequence头数据
extern "C"
JNIEXPORT jint JNICALL
Java_com_live_LiveNativeManager_sendRtmpAudioSpec(JNIEnv *env, jclass type, jlong timeStamp) {

    if (rtmpLivePush) {
        // 直接指定为44100采样率，2声道
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
        // c++ 中没有byte类型，用unsigned char代替
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
        //delete (audioEncoder);
        audioEncoder->close();
    }
    return 0;
}

/**
 * 音频编码结果的回调函数
 * @param data
 * @param dataLen
 */
void audioEncoderCallback(uint8_t *data, int dataLen) {
    AvData *avData = static_cast<AvData *>(malloc(sizeof(AvData)));
    avData->data = data;
    avData->len = dataLen;
    avData->type = AUDIO;
    avData->nalNums = 0;
    avData->nalSizes = NULL;
    //  保存到推流器
    if (rtmpLivePush) {
        rtmpLivePush->putAvData(avData);
    } else {
        free(avData);
    }
}

/**
 * YUV数据处理回调
 * @param data
 * @param dataLen
 */
void yuvProcessCallback(uint8_t *data, int dataLen) {
    AvData *avData = static_cast<AvData *>(malloc(sizeof(AvData)));
    avData->data = data;
    avData->len = dataLen;
    avData->type = VIDEO;
    avData->nalNums = 0;
    avData->nalSizes = NULL;
    //  保存视频编码器
    if (videoEncoder) {
        videoEncoder->putAvData(avData);
    } else {
        free(avData);
    }
}

void videoEncoderCallback(uint8_t *data, int dataLen, int nalNum, int *nalsSize) {
    AvData *avData = static_cast<AvData *>(malloc(sizeof(AvData)));
    avData->data = data;
    avData->len = dataLen;
    avData->nalNums = nalNum;
    avData->nalSizes = nalsSize;
    avData->type = VIDEO;
    //  save
    if (rtmpLivePush) {
        for (int i = 0; i < avData->nalNums; i++) {
            LOGD("保存nal大小：%d", avData->nalSizes[i]);
        }
        rtmpLivePush->putAvData(avData);
    } else {
        free(avData);
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_startAudioEncode(JNIEnv *env, jclass clazz) {
    if (audioEncoder) {
        audioEncoder->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_stopAudioEncode(JNIEnv *env, jclass clazz) {
    if (audioEncoder)
        audioEncoder->stop();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_pauseAudioEncode(JNIEnv *env, jclass clazz) {
    audioEncoder->pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_resumeAudioEncode(JNIEnv *env, jclass clazz) {
    audioEncoder->resume();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_putAudioData(JNIEnv *env, jclass clazz, jbyteArray audio_data, jint data_len) {
    //  转为jbyte数组的指针
    jbyte *srcData = env->GetByteArrayElements(audio_data, NULL);
    //  保存音频数据
    audioEncoder->putAudioData((uint8_t *) srcData, data_len);

    env->ReleaseByteArrayElements(audio_data, srcData, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_initVideoEncoder(JNIEnv *env, jclass clazz, jint video_with, jint video_height,
                                                 jint scale_width, jint scale_height, jint orientation) {
    //  初始化YUV处理器
    yuvProcess = new YuvProcess();
    yuvProcess->setSrcWH(video_with, video_height);
    yuvProcess->setScaleWH(scale_width, scale_height);
    yuvProcess->setDegress(orientation);
    yuvProcess->init();
    yuvProcess->setYuvCallback(&yuvProcessCallback); //  回调函数

    //  初始化视频编码器
    videoEncoder = new VideoEncoder();
    // 设置相关参数
    videoEncoder->setInWidth(scale_width);
    videoEncoder->setInHeight(scale_height);
    videoEncoder->setBitrate(1200 * 1000); // 设置比特率
    videoEncoder->open(); //  打开编码器
    videoEncoder->setVideoEncoderCallback(&videoEncoderCallback); //  回调函数
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_putVideoData(JNIEnv *env, jclass clazz, jbyteArray video_data, jint data_len) {
    //  转为jbyte数组的指针
    jbyte *srcData = env->GetByteArrayElements(video_data, NULL);
    //  保存数据
    if (yuvProcess) {
        yuvProcess->putAvData((uint8_t *) srcData, data_len);
    }
    env->ReleaseByteArrayElements(video_data, srcData, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_startVideoEncoder(JNIEnv *env, jclass clazz) {
    // start video encode
    yuvProcess->start();
    videoEncoder->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_stopVideoEncoder(JNIEnv *env, jclass clazz) {
    if (yuvProcess)
        yuvProcess->stop();
    if (videoEncoder)
        videoEncoder->stop();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_pauseVideoEncoder(JNIEnv *env, jclass clazz) {
    yuvProcess->pause();
    videoEncoder->pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_resumeVideoEncoder(JNIEnv *env, jclass clazz) {
    yuvProcess->resume();
    videoEncoder->resume();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_startRtmpPublish(JNIEnv *env, jclass clazz) {
    rtmpLivePush->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_stopRtmpPublish(JNIEnv *env, jclass clazz) {
    if (rtmpLivePush)
        rtmpLivePush->stop();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_pauseRtmpPublish(JNIEnv *env, jclass clazz) {
    rtmpLivePush->pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_live_LiveNativeManager_resumeRtmpPublish(JNIEnv *env, jclass clazz) {
    rtmpLivePush->resume();
}