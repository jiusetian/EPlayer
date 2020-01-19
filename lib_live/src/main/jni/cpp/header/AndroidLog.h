
#ifndef NATIVE_LOG_H
#define NATIVE_LOG_H

#include <android/log.h>

#define JNI_TAG "ELIVE"

#if defined(__ANDROID__)

#define LOGE(format, ...) __android_log_print(ANDROID_LOG_ERROR, JNI_TAG, format, ##__VA_ARGS__)
#define LOGI(format, ...) __android_log_print(ANDROID_LOG_INFO,  JNI_TAG, format, ##__VA_ARGS__)
#define LOGD(format, ...) __android_log_print(ANDROID_LOG_DEBUG, JNI_TAG, format, ##__VA_ARGS__)
#define LOGW(format, ...) __android_log_print(ANDROID_LOG_WARN,  JNI_TAG, format, ##__VA_ARGS__)
#define LOGV(format, ...) __android_log_print(ANDROID_LOG_VERBOSE,  JNI_TAG, format, ##__VA_ARGS__)
#else

#define LOGE(format, ...) {}
#define LOGI(format, ...) {}
#define LOGD(format, ...) {}
#define LOGW(format, ...) {}
#define LOGV(format, ...) {}
#endif

#endif //NATIVE_LOG_H
