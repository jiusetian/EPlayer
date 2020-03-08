
#include <unistd.h>
#include <jni.h>
#include <AndroidLog.h>
#include <string.h>

#include "MediaMetadataRetriever.h"

extern "C" {
#include <libavcodec/jni.h>
};


struct retriever_fields_t {
    jfieldID context;
};

static retriever_fields_t fields;
static Mutex sLock;
static const char *const RETRIEVER_CLASS_NAME = "com/eplayer/EMediaMetadataRetriever";

// -------------------------------------------------------------------------------------------------

/**
 * 创建新的jstring对象
 * @param env
 * @param data
 * @return
 */
static jstring newUTFString(JNIEnv *env, const char *data) {
    jstring str = NULL;

    int size = strlen(data);

    jbyteArray array = NULL;
    //创建一个字节数组
    array = env->NewByteArray(size);
    if (!array) {
        LOGE("convertString: OutOfMemoryError is thrown.");
    } else {
        //获取字节数组的指针
        jbyte *bytes = env->GetByteArrayElements(array, NULL);
        if (bytes != NULL) {
            //将data的数据复制到数组中
            memcpy(bytes, data, size);
            env->ReleaseByteArrayElements(array, bytes, 0);
            //调用java的方法创建一个string对象
            jclass clazz = env->FindClass("java/lang/String");
            jmethodID pMethodID = env->GetMethodID(clazz, "<init>", "([BLjava/lang/String;)V");
            jstring utf = env->NewStringUTF("UTF-8");
            str = (jstring) env->NewObject(clazz, pMethodID, array, utf);
            //释放某个局部引用
            env->DeleteLocalRef(utf);
        }
    }
    env->DeleteLocalRef(array);

    return str;
}

/**
 * 抛出异常
 * @param env
 * @param className
 * @param msg
 */
void throwException(JNIEnv *env, const char *className, const char *msg) {
    jclass exception = env->FindClass(className);
    env->ThrowNew(exception, msg);
}

static int getFDFromFileDescriptor(JNIEnv *env, jobject fileDescriptor) {
    jint fd = -1;
    jclass fdClass = env->FindClass("java/io/FileDescriptor");

    if (fdClass != NULL) {
        jfieldID fdClassDescriptorFieldID = env->GetFieldID(fdClass, "descriptor", "I");
        if (fdClassDescriptorFieldID != NULL && fileDescriptor != NULL) {
            fd = env->GetIntField(fileDescriptor, fdClassDescriptorFieldID);
        }
    }

    return fd;
}

static MediaMetadataRetriever *getRetriever(JNIEnv *env, jobject thiz) {
    MediaMetadataRetriever *retriever = (MediaMetadataRetriever *) env->GetLongField(thiz, fields.context);
    return retriever;
}

static void setRetriever(JNIEnv *env, jobject thiz, long retriever) {
    MediaMetadataRetriever *old = (MediaMetadataRetriever *) env->GetLongField(thiz, fields.context);
    env->SetLongField(thiz, fields.context, retriever);
}

// -------------------------------------------------------------------------------------------------
static void process_media_retriever_call(JNIEnv *env, status_t opStatus, const char *exception, const char *message) {
    if (opStatus == (status_t) INVALID_OPERATION) {
        throwException(env, "java/lang/IllegalStateException", NULL);
    } else if (opStatus != (status_t) OK) {
        if (strlen(message) > 230) {
            throwException(env, exception, message);
        } else {
            char msg[256];
            sprintf(msg, "%s: status = 0x%X", message, opStatus);
            throwException(env, exception, msg);
        }
    }
}

static void EMediaMetadataRetriever_setDataSourceAndHeaders(JNIEnv *env, jobject thiz, jstring path_, jobjectArray keys,
                                                            jobjectArray values) {

    LOGV("setDataSource");
    MediaMetadataRetriever *retriever = getRetriever(env, thiz);
    if (retriever == NULL) {
        throwException(env, "java/lang/IllegalStateException", "No retriever available");
        return;
    }

    if (!path_) {
        throwException(env, "java/lang/IllegalArgumentException", "Null pointer");
        return;
    }

    //将 Java 风格的 jstring 对象转换成 C 风格的字符串
    const char *path = env->GetStringUTFChars(path_, NULL);
    if (!path) {
        return;
    }

    // C 库函数 int strncmp(const char *str1, const char *str2, size_t n) 把 str1 和 str2 进行比较，最多比较前 n 个字节
    // 字符串比较函数，字符串大小的比较是以ASCII 码表上的顺序来决定，此顺序亦为字符的值
    // str1 -- 要进行比较的第一个字符串
    // str2 -- 要进行比较的第二个字符串
    // n -- 要比较的最大字符数

    //该函数返回值如下：
    //    如果返回值 < 0，则表示 str1 小于 str2
    //    如果返回值 > 0，则表示 str2 小于 str1
    //    如果返回值 = 0，则表示 str1 等于 str2
    if (strncmp("mem://", path, 6) == 0) {
        throwException(env, "java/lang/IllegalArgumentException", "Invalid pathname");
        return;
    }

    const char *restrict = strstr(path, "mms://");
    char *restrict_to = restrict ? strdup(restrict) : NULL;
    if (restrict_to != NULL) {
        strncpy(restrict_to, "mmsh://", 6);
        puts(path);
    }

    char *headers = NULL;
    if (keys && values != NULL) {
        int keysCount = env->GetArrayLength(keys);
        int valuesCount = env->GetArrayLength(values);

        if (keysCount != valuesCount) {
            LOGE("keys and values arrays have different length");
            throwException(env, "java/lang/IllegalArgumentException", NULL);
            return;
        }

        int i = 0;
        const char *rawString = NULL;
        char hdrs[2048];

        for (i = 0; i < keysCount; i++) {
            jstring key = (jstring) env->GetObjectArrayElement(keys, i);
            rawString = env->GetStringUTFChars(key, NULL);
            strcat(hdrs, rawString);
            strcat(hdrs, ": ");
            env->ReleaseStringUTFChars(key, rawString);

            jstring value = (jstring) env->GetObjectArrayElement(values, i);
            rawString = env->GetStringUTFChars(value, NULL);
            strcat(hdrs, rawString);
            strcat(hdrs, "\r\n");
            env->ReleaseStringUTFChars(value, rawString);
        }

        headers = &hdrs[0];
    }
    //设置数据源
    status_t opStatus = retriever->setDataSource(path, 0, headers);
    //异常检查相关
    process_media_retriever_call(env, opStatus, "java/io/IOException", "setDataSource failed.");

    env->ReleaseStringUTFChars(path_, path);
}

static void
EMediaMetadataRetriever_setDataSource(JNIEnv *env, jobject thiz, jstring path_) {
    EMediaMetadataRetriever_setDataSourceAndHeaders(env, thiz, path_, NULL, NULL);
}


static void
EMediaMetadataRetriever_setDataSourceFD(JNIEnv *env, jobject thiz, jobject fileDescriptor, jlong offset, jlong length) {
    MediaMetadataRetriever *retriever = getRetriever(env, thiz);
    if (retriever == NULL) {
        throwException(env, "java/lang/IllegalStateException", "No retriever available");
        return;
    }
    if (!fileDescriptor) {
        throwException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    int fd = getFDFromFileDescriptor(env, fileDescriptor);
    if (offset < 0 || length < 0 || fd < 0) {
        if (offset < 0) {
            LOGE("negative offset (%lld)", offset);
        }
        if (length < 0) {
            LOGE("negative length (%lld)", length);
        }
        if (fd < 0) {
            LOGE("invalid file descriptor");
        }
        throwException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    char path[256] = "";
    int myfd = dup(fd);
    char str[20];
    sprintf(str, "pipe:%d", myfd);
    strcat(path, str);

    status_t opStatus = retriever->setDataSource(path, offset, NULL);
    process_media_retriever_call(env, opStatus, "java/io/IOException", "setDataSourceFD failed.");

}

static jbyteArray EMediaMetadataRetriever_getFrameAtTime(JNIEnv *env, jobject thiz, jlong timeUs, jint option) {

    MediaMetadataRetriever *retriever = getRetriever(env, thiz);
    if (retriever == NULL) {
        throwException(env, "java/lang/IllegalStateException", "No retriever available");
        return NULL;
    }

    AVPacket packet;
    av_init_packet(&packet);
    jbyteArray array = NULL;
    //取得某时刻的图像
    if (retriever->getFrame(timeUs, &packet) == 0) {
        int size = packet.size;
        uint8_t *data = packet.data;
        array = env->NewByteArray(size);

        jbyte *bytes = env->GetByteArrayElements(array, NULL);
        if (bytes != NULL) {
            memcpy(bytes, data, size);
            env->ReleaseByteArrayElements(array, bytes, 0);
        }
    }
    av_packet_unref(&packet);

    return array;
}

static jbyteArray
EMediaMetadataRetriever_getScaleFrameAtTime(JNIEnv *env, jobject thiz, jlong timeUs, jint option, jint width,
                                            jint height) {

    MediaMetadataRetriever *retriever = getRetriever(env, thiz);
    if (retriever == NULL) {
        throwException(env, "java/lang/IllegalStateException", "No retriever available");
        return NULL;
    }

    AVPacket packet;
    av_init_packet(&packet);
    jbyteArray array = NULL;
    //取得某个时刻的图像
    if (retriever->getFrame(timeUs, &packet, width, height) == 0) { //返回0代表成功
        int size = packet.size;
        uint8_t *data = packet.data;
        array = env->NewByteArray(size);

        jbyte *bytes = env->GetByteArrayElements(array, NULL);
        if (bytes != NULL) {
            memcpy(bytes, data, size);
            env->ReleaseByteArrayElements(array, bytes, 0);
        }
    }
    av_packet_unref(&packet);
    return array;

}

static jbyteArray EMediaMetadataRetriever_getEmbeddedPicture(JNIEnv *env, jobject thiz, jint pictureType) {

    MediaMetadataRetriever *retriever = getRetriever(env, thiz);
    if (retriever == NULL) {
        throwException(env, "java/lang/IllegalStateException", "No retriever available");
        return NULL;
    }

    AVPacket packet;
    av_init_packet(&packet);
    jbyteArray array = NULL;
    //提取封面专辑图片
    if (retriever->getEmbeddedPicture(&packet) == 0) {
        int size = packet.size;
        uint8_t *data = packet.data;
        //创建基本数据类型的数组
        array = env->NewByteArray(size);
        //获取数组指针
        jbyte *bytes = env->GetByteArrayElements(array, NULL);
        if (bytes != NULL) {
            //将data数据拷贝到bytes
            memcpy(bytes, data, size);
            env->ReleaseByteArrayElements(array, bytes, 0);
        }
    }
    av_packet_unref(&packet);
    return array;
}

static jstring EMediaMetadataRetriever_extractMetadata(JNIEnv *env, jobject thiz, jstring key_) {

    MediaMetadataRetriever *retriever = getRetriever(env, thiz);
    if (retriever == NULL) {
        throwException(env, "java/lang/IllegalStateException", "No retriever available");
        return NULL;
    }

    if (!key_) {
        throwException(env, "java/lang/IllegalArgumentException", "Null pointer");
        return NULL;
    }
    const char *key = env->GetStringUTFChars(key_, 0);
    if (!key) {
        return NULL;
    }

    //提取metadata数据
    const char *value = retriever->getMetadata(key);
    if (!value) {
        return NULL;
    }

    env->ReleaseStringUTFChars(key_, key);

    return newUTFString(env, value);
}

static jstring
EMediaMetadataRetriever_extractMetadataFromChapter(JNIEnv *env, jobject thiz, jstring key_, jint chapter) {
    MediaMetadataRetriever *retriever = getRetriever(env, thiz);
    if (retriever == NULL) {
        throwException(env, "java/lang/IllegalStateException", "No retriever available");
        return NULL;
    }

    if (!key_) {
        throwException(env, "java/lang/IllegalArgumentException", "Null pointer");
        return NULL;
    }
    const char *key = env->GetStringUTFChars(key_, 0);
    if (!key) {
        return NULL;
    }

    const char *value = retriever->getMetadata(key, chapter);
    if (!value) {
        return NULL;
    }

    env->ReleaseStringUTFChars(key_, key);

    return newUTFString(env, value);
}

static jobject EMediaMetadataRetriever_getAllMetadata(JNIEnv *env, jobject thiz) {

    MediaMetadataRetriever *retriever = getRetriever(env, thiz);
    if (retriever == NULL) {
        throwException(env, "java/lang/IllegalStateException", "No retriever available");
        return NULL;
    }

    AVDictionary *metadata = NULL;
    int ret = retriever->getMetadata(&metadata);
    if (ret == 0) {
        //获得hashmap的class对象
        jclass hashMapClass = env->FindClass("java/util/HashMap");
        //获取类中某个非静态方法的ID
        jmethodID hashMap_init = env->GetMethodID(hashMapClass, "<init>", "()V");
        // 创建一个HashMap对象
        jobject hashMap = env->NewObject(hashMapClass, hashMap_init); //使用指定的构造方法创建类的实例
        jmethodID hashMap_put = env->GetMethodID(hashMapClass, "put",
                                                 "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
        // 将metadata的参数复制到HashMap中
        for (int i = 0; i < metadata->count; ++i) {
            jstring key = newUTFString(env, metadata->elements[i].key);
            jstring value = newUTFString(env, metadata->elements[i].value);
            env->CallObjectMethod(hashMap, hashMap_put, key, value);
        }

        if (metadata) {
            av_dict_free(&metadata);
        }

        return hashMap;
    }
}

static void EMediaMetadataRetriever_setup(JNIEnv *env, jobject thiz) {
    MediaMetadataRetriever *retriever = new MediaMetadataRetriever();
    if (retriever == NULL) {
        throwException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }
    setRetriever(env, thiz, (long) retriever);
}

static void EMediaMetadataRetriever_release(JNIEnv *env, jobject thiz) {
    Mutex::Autolock lock(sLock);
    MediaMetadataRetriever *retriever = getRetriever(env, thiz);
    delete retriever;
    setRetriever(env, thiz, 0);
}

static void EMediaMetadataRetriever_native_finalize(JNIEnv *env, jobject thiz) {
    LOGV("native_finalize");
    EMediaMetadataRetriever_release(env, thiz);
}

static void EMediaMetadataRetriever_native_init(JNIEnv *env) {
    jclass clazz = env->FindClass(RETRIEVER_CLASS_NAME);
    if (clazz == NULL) {
        return;
    }
    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    if (fields.context == NULL) {
        return;
    }
}

static JNINativeMethod nativeMethods[] = {
        {"setDataSource",              "(Ljava/lang/String;)V",                   (void *) EMediaMetadataRetriever_setDataSource},
        {
         "_setDataSource",
                                       "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V",
                                                                                  (void *) EMediaMetadataRetriever_setDataSourceAndHeaders
        },
        {"setDataSource",              "(Ljava/io/FileDescriptor;JJ)V",           (void *) EMediaMetadataRetriever_setDataSourceFD},

        {"_getFrameAtTime",            "(JI)[B",                                  (void *) EMediaMetadataRetriever_getFrameAtTime},
        {"_getScaledFrameAtTime",      "(JIII)[B",                                (void *) EMediaMetadataRetriever_getScaleFrameAtTime},
        {"getEmbeddedPicture",         "(I)[B",                                   (void *) EMediaMetadataRetriever_getEmbeddedPicture},

        {"extractMetadata",            "(Ljava/lang/String;)Ljava/lang/String;",  (void *) EMediaMetadataRetriever_extractMetadata},
        {"extractMetadataFromChapter", "(Ljava/lang/String;I)Ljava/lang/String;", (void *) EMediaMetadataRetriever_extractMetadataFromChapter},

        {"_getAllMetadata",            "()Ljava/util/HashMap;",                   (void *) EMediaMetadataRetriever_getAllMetadata},

        {"release",                    "()V",                                     (void *) EMediaMetadataRetriever_release},
        {"native_setup",               "()V",                                     (void *) EMediaMetadataRetriever_setup},
        {"native_init",                "()V",                                     (void *) EMediaMetadataRetriever_native_init},
        {"native_finalize",            "()V",                                     (void *) EMediaMetadataRetriever_native_finalize},

};

// 注册Native方法
static int register_eplayer_EMediaMetadataRetriever(JNIEnv *env) {
    int numMethods = (sizeof(nativeMethods) / sizeof((nativeMethods)[0]));
    jclass clazz = env->FindClass(RETRIEVER_CLASS_NAME);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'", RETRIEVER_CLASS_NAME);
        return JNI_ERR;
    }
    if (env->RegisterNatives(clazz, nativeMethods, numMethods) < 0) {
        LOGE("Native registration unable to find class '%s'", RETRIEVER_CLASS_NAME);
        return JNI_ERR;
    }
    env->DeleteLocalRef(clazz);

    return JNI_OK;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    av_jni_set_java_vm(vm, NULL);
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("本地方法注册成功");
        return -1;
    }
    if (register_eplayer_EMediaMetadataRetriever(env) != JNI_OK) {
        LOGE("本地方法注册失败");
        return -1;
    }
    return JNI_VERSION_1_4;
}