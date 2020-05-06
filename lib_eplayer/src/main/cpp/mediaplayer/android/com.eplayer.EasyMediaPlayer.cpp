
#include <jni.h>
#include <Mutex.h>
#include <Condition.h>
#include <Errors.h>
#include <JNIHelp.h>
#include <EMediaPlayer.h>
#include <cstring>
#include <cstdio>
#include <memory.h>

extern "C" {
#include <libavcodec/jni.h>
}

const char *CLASS_NAME = "com/eplayer/EasyMediaPlayer";

// -------------------------------------------------------------------------------------------------
struct fields_t {
    jfieldID context; //c++层的easymediaplayer实例在java层的引用的id值
    jmethodID post_event; //java层信息回调函数的函数ID
};
//结构体引用，声明的时候已经分配好内存，不用手动分配和释放内存空间
static fields_t fields;

static JavaVM *javaVM = NULL;

static JNIEnv *getJNIEnv() {

    JNIEnv *env;
    //assert的作用是先计算()里面的表达式 ，若其值为假（即为0），那么它先向stderr打印一条出错信息，然后通过调用abort来终止程序运行
    assert(javaVM != NULL);
    //因为要通过GetEnv给指针env赋值，所以用&env，指针引用就是指针的指针
    if (javaVM->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return NULL;
    }
    return env;
}

// -------------------------------------------------------------------------------------------------
class JNIMediaPlayerListener : public MediaPlayerListener {
public:
    JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_thiz);

    ~JNIMediaPlayerListener();

    //后面用override，代表重写基类的虚函数
    void notify(int msg, int ext1, int ext2, void *obj) override;

private:
    //将无参构造函数设置为私有，不被外界调用
    JNIMediaPlayerListener();

    jclass mClass; //java层easymediaplayer类的class实例
    //cpp层对java层的easymediaplayer实例的弱引用，因为是弱引用，所以当java层的实例要销毁即不再有强引用的时候，这个弱引用不会造成内存泄漏
    //回调消息通知notify方法的时候，传递这个弱引用给java层，在java层再通过调用对应的方法通知消息
    jobject mObject;
};

//定义构造函数
JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_thiz) {
    // Hold onto the MediaPlayer class for use in calling the static method
    // that posts events to the application thread.
    //获取class对象，参数为java的对象实例
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        LOGE("Can't find com/eplayer/EMediaPlayer");
        jniThrowException(env, "java/lang/Exception");
        return;
    }
    //创建一个全局引用，不用时必须调用 DeleteGlobalRef() 方法释放，参数为某个Java 对象实例，可以是局部引用或全局引用
    mClass = (jclass) env->NewGlobalRef(clazz);

    // We use a weak reference so the MediaPlayer object can be garbage collected
    // The reference is only used as a proxy for callbacks
    mObject = env->NewGlobalRef(weak_thiz);
}

//定义析构函数
JNIMediaPlayerListener::~JNIMediaPlayerListener() {
    JNIEnv *env = getJNIEnv();
    //释放全局引用
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
}

//ext1和ext2对应msg的arg1和arg2，即仅传递低成本的int值，obj代表传递一个对象
void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2, void *obj) {
    JNIEnv *env = getJNIEnv();
    //获取对应线程的JNIEnv对象，将当前线程跟JVM关联
    bool status = (javaVM->AttachCurrentThread(&env, NULL) >= 0);

    // TODO obj needs changing into jobject
    //调用java层的static方法，mObject, msg, ext1, ext2, obj 均为方法参数，对应java层handler的方法obtainMessage(what, arg1, arg2, obj)，msg为消息ID
    //调用java层的static方法，关键需要java层对应类的class对象和方法ID，后面就是参数
    env->CallStaticVoidMethod(mClass, fields.post_event, mObject, msg, ext1, ext2, obj);

    if (env->ExceptionCheck()) {
        LOGW("An exception occurred while notifying an event.");
        env->ExceptionClear();
    }
    //记得解除链接
    if (status) {
        javaVM->DetachCurrentThread();
    }
}


// -------------------------------------------------------------------------------------------------

static EMediaPlayer *getMediaPlayer(JNIEnv *env, jobject thiz) {
    //获取java层保存的c++的emediaplayer的实例引用，然后将其转换为EMediaPlayer * 指针的类型
    EMediaPlayer *const mp = (EMediaPlayer *) env->GetLongField(thiz, fields.context);
    return mp;
}

/**
 * 将c++的mediaplayer对象保存到java层，其中fields.context是java层的变量的ID，是一个long类型的变量
 */
static EMediaPlayer *setMediaPlayer(JNIEnv *env, jobject thiz, long mediaPlayer) {
    EMediaPlayer *old = (EMediaPlayer *) env->GetLongField(thiz, fields.context);
    env->SetLongField(thiz, fields.context, mediaPlayer);
    return old;
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(JNIEnv *env, jobject thiz, int opStatus,
                                      const char *exception, const char *message) {
    if (exception == NULL) {  // Don't throw exception. Instead, send an event.
        if (opStatus != (int) OK) {
            EMediaPlayer *mp = getMediaPlayer(env, thiz);
            if (mp != 0) mp->notify(MEDIA_ERROR, opStatus, 0);
        }
    } else {  // Throw exception!
        if (opStatus == (int) INVALID_OPERATION) {
            jniThrowException(env, "java/lang/IllegalStateException");
        } else if (opStatus == (int) PERMISSION_DENIED) {
            jniThrowException(env, "java/lang/SecurityException");
        } else if (opStatus != (int) OK) {
            if (strlen(message) > 230) {
                // if the message is too long, don't bother displaying the status code
                jniThrowException(env, exception, message);
            } else {
                char msg[256];
                // append the status code to the message
                sprintf(msg, "%s: status=0x%X", message, opStatus);
                jniThrowException(env, exception, msg);
            }
        }
    }
}

void EMediaPlayer_setDataSourceAndHeaders(JNIEnv *env, jobject thiz, jstring path_,
                                          jobjectArray keys, jobjectArray values) {

    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }

    if (path_ == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException");
        return;
    }
    //将 Java 风格的 jstring 对象转换成 C 风格的字符串，0为不拷贝
    const char *path = env->GetStringUTFChars(path_, 0);
    if (path == NULL) {
        return;
    }
    //MMS协议：MMS（MicrosoftMediaServerprotocol）是一种串流媒体传送协议，用来访问并流式接收Windows Media服务器中.asf文件的一种协议
    //strstr(str1,str2) 函数用于判断字符串str2是否是str1的子串。如果是，则该函数返回str1字符串从str2第一次出现的位置开始到str1结尾的字符串；否则，返回NULL
    const char *restrict = strstr(path, "mms://"); //是否为mms协议的多媒体
    //strdup是复制字符串，返回一个指针,指向为复制字符串分配的空间;如果分配空间失败,则返回NULL值
    char *restrict_to = restrict ? strdup(restrict) : NULL;
    if (restrict_to != NULL) {
        //把src所指向的字符串复制到dest，最多复制n个字符。当src的长度小于n时，dest的剩余部分将用空字节填充
        strncpy(restrict_to, "mmsh://", 6);
        //将path输出到屏幕
        puts(path);
    }

    char *headers = NULL;
    if (keys && values != NULL) {
        int keysCount = env->GetArrayLength(keys);
        int valuesCount = env->GetArrayLength(values);

        if (keysCount != valuesCount) {
            LOGE("keys and values arrays have different length");
            jniThrowException(env, "java/lang/IllegalArgumentException");
            return;
        }

        int i = 0;
        const char *rawString = NULL;
        char hdrs[2048];

        for (i = 0; i < keysCount; i++) {
            jstring key = (jstring) env->GetObjectArrayElement(keys, i);
            rawString = env->GetStringUTFChars(key, NULL);
            //char *strcat(char *dest, const char *src) 把src所指向的字符串追加到dest所指向的字符串的结尾
            strcat(hdrs, rawString);
            strcat(hdrs, ": ");
            env->ReleaseStringUTFChars(key, rawString);

            jstring value = (jstring) env->GetObjectArrayElement(values, i);
            rawString = env->GetStringUTFChars(value, NULL);
            strcat(hdrs, rawString);
            strcat(hdrs, "\r\n"); //回车换行
            env->ReleaseStringUTFChars(value, rawString);
        }

        headers = &hdrs[0];
    }

    status_t opStatus = mp->setDataSource(path, 0, headers);
    process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSource failed.");

    env->ReleaseStringUTFChars(path_, path);
}

void EMediaPlayer_changeFilter(JNIEnv *env, jobject thiz, int node_type, jstring filterName_) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    const char *name = env->GetStringUTFChars(filterName_, NULL);

    mp->changeFilter(node_type, name);

    env->ReleaseStringUTFChars(filterName_, name);
}

void EMediaPlayer_setWatermark(JNIEnv *env, jobject thiz, jbyteArray data_,
                               jint dataLen, jint watermarkWidth,
                               jint watermarkHeight,jfloat scale,jint location){
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }

    jbyte *data = env->GetByteArrayElements(data_, NULL);

    mp->setWatermark((uint8_t *)data,(size_t) dataLen, watermarkWidth, watermarkHeight,scale, location);

    env->ReleaseByteArrayElements(data_, data, 0);
}


void EMediaPlayer_changeFilterById(JNIEnv *env, jobject thiz, int node_type, jint filterId) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }

    mp->changeFilter(node_type, filterId);
}

void EMediaPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path_) {
    EMediaPlayer_setDataSourceAndHeaders(env, thiz, path_, NULL, NULL);
}

void EMediaPlayer_setDataSourceFD(JNIEnv *env, jobject thiz, jobject fileDescriptor,
                                  jlong offset, jlong length) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }

    if (fileDescriptor == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException");
        return;
    }

    int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
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
        jniThrowException(env, "java/lang/IllegalArgumentException");
        return;
    }

    char path[256] = "";
    int myfd = dup(fd);
    char str[20];
    sprintf(str, "pipe:%d", myfd);
    strcat(path, str);

    status_t opStatus = mp->setDataSource(path, offset, NULL);
    process_media_player_call(env, thiz, opStatus, "java/io/IOException", "setDataSourceFD failed.");

}

//主要获取一个java层的mediaplayer的一个变量ID和一个方法ID，这个方法在java层static块中执行，比构造方法先执行，
// 类的static代码块只在第一次调用构造方法适合执行，第二次将不再执行static代码块了
void EMediaPlayer_init(JNIEnv *env) {

    //根据类的全路径找到相应的 jclass 对象
    jclass clazz = env->FindClass(CLASS_NAME);
    if (clazz == NULL) {
        return;
    }

    //获取类中某个非静态成员变量的ID（域ID）
    //    参数：
    //    clazz：指定对象的类
    //    name：这个域（Field）在 Java 类中定义的名字
    //    sig：这个域（Field）的类型描述符
    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");

    if (fields.context == NULL) {
        return;
    }

    //获取类中某个静态方法的ID
    //    clazz：指定对象的类
    //    name：这个方法在 Java 类中定义的名称，构造方法为
    //    sig：这个方法的类型描述符，例如 “()V”，其中括号内是方法的参数，括号后是返回值类型
    //在java中函数名为postEventFromNative，参数类型有object、int、int、int、object四个，返回值为void
    // private static void postEventFromNative(Object mediaplayer_ref,int what, int arg1, int arg2, Object obj)
    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        return;
    }

    //释放某个局部引用
    //局部引用在方法执行完后也会自动释放，不过当你在执行一个很大的循环时，里面会产生大量临时的局部引用，
    // 那么建议的做法是手动的调用该方法去释放这个局部引用
    env->DeleteLocalRef(clazz);
}

/**
 * java层的mediaplayer构造方法中会调用此方法，在这里会创建c++层的mediaplayer对象，设置mediaplayer消息的监听，然后保持cpp层mediaplayer
 * 对象的引用到java层的mediaplayer对象中
 * @param env
 * @param thiz
 * @param mediaplayer_this
 */
void EMediaPlayer_setup(JNIEnv *env, jobject thiz, jobject mediaplayer_this) {

    EMediaPlayer *mp = new EMediaPlayer();
    if (mp == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }

    // 这里似乎存在问题
    // init EMediaPlayer
    mp->init();

    // create new listener and give it to MediaPlayer
    JNIMediaPlayerListener *listener = new JNIMediaPlayerListener(env, thiz, mediaplayer_this);
    //设置监听
    mp->setListener(listener);

    // Stow our new C++ MediaPlayer in an opaque field in the Java object.其中opaque field 不透明的变量，即私有变量
    setMediaPlayer(env, thiz, (long) mp);
}

void EMediaPlayer_release(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp != nullptr) {
        mp->disconnect();
        delete mp;
        setMediaPlayer(env, thiz, 0);
    }
}

void EMediaPlayer_reset(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->reset();
}

void EMediaPlayer_finalize(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp != NULL) {
        LOGW("MediaPlayer finalized without being released");
    }
    LOGD("调用完成");
    EMediaPlayer_release(env, thiz);
}

void EMediaPlayer_setVideoSurface(JNIEnv *env, jobject thiz, jobject surface) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    ANativeWindow *window = NULL;
    if (surface != NULL) {
        //将java层Surface转换为C++的ANativeWindow
        window = ANativeWindow_fromSurface(env, surface);
    }
    mp->setVideoSurface(window);
}

void EMediaPlayer_setLooping(JNIEnv *env, jobject thiz, jboolean looping) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setLooping(looping);
}

jboolean EMediaPlayer_isLooping(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return JNI_FALSE;
    }
    return (jboolean) (mp->isLooping() ? JNI_TRUE : JNI_FALSE);
}

void EMediaPlayer_prepare(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }

    mp->prepare();
}

void EMediaPlayer_prepareAsync(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->prepareAsync();
}

void EMediaPlayer_start(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->start();
}

void EMediaPlayer_pause(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->pause();
}

void EMediaPlayer_resume(JNIEnv *env, jobject thiz) {

    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->resume();

}

void EMediaPlayer_stop(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->stop();
}

void EMediaPlayer_seekTo(JNIEnv *env, jobject thiz, jfloat timeMs) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->seekTo(timeMs);
}

void EMediaPlayer_setMute(JNIEnv *env, jobject thiz, jboolean mute) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setMute(mute);
}

void EMediaPlayer_setVolume(JNIEnv *env, jobject thiz, jfloat leftVolume, jfloat rightVolume) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setVolume(leftVolume, rightVolume);
}

void EMediaPlayer_setRate(JNIEnv *env, jobject thiz, jfloat speed) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setRate(speed);
}

void EMediaPlayer_setPitch(JNIEnv *env, jobject thiz, jfloat pitch) {

    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    mp->setPitch(pitch);
}

jlong EMediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz) {

    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0L;
    }
    return mp->getCurrentPosition();
}

jlong EMediaPlayer_getDuration(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0L;
    }
    return mp->getDuration();
}

jboolean EMediaPlayer_isPlaying(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return JNI_FALSE;
    }
    return (jboolean) (mp->isPlaying() ? JNI_TRUE : JNI_FALSE);
}

jint EMediaPlayer_getRotate(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0;
    }
    return mp->getRotate();
}

jint EMediaPlayer_getVideoWidth(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0;
    }
    return mp->getVideoWidth();
}

jint EMediaPlayer_getVideoHeight(JNIEnv *env, jobject thiz) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return 0;
    }
    return mp->getVideoHeight();
}

void EMediaPlayer_surfaceChange(JNIEnv *env, jobject thiz,jint width, jint height){
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }

    mp->surfaceChanged(width,height);
}

void EMediaPlayer_setOption(JNIEnv *env, jobject thiz, int category, jstring type_, jstring option_) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    const char *type = env->GetStringUTFChars(type_, 0);
    const char *option = env->GetStringUTFChars(option_, 0);
    if (type == NULL || option == NULL) {
        return;
    }

    mp->setOption(category, type, option);

    env->ReleaseStringUTFChars(type_, type);
    env->ReleaseStringUTFChars(option_, option);
}

void EMediaPlayer_setOptionLong(JNIEnv *env, jobject thiz, int category, jstring type_, jlong option_) {
    EMediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException");
        return;
    }
    const char *type = env->GetStringUTFChars(type_, 0);
    if (type == NULL) {
        return;
    }
    mp->setOption(category, type, option_);

    env->ReleaseStringUTFChars(type_, type);
}


static const JNINativeMethod gMethods[] = {
        {"_setDataSource",      "(Ljava/lang/String;)V",                    (void *) EMediaPlayer_setDataSource},
        {"_setDataSource",      "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V",
                                                                            (void *) EMediaPlayer_setDataSourceAndHeaders},
        {"_setDataSource",      "(Ljava/io/FileDescriptor;JJ)V",            (void *) EMediaPlayer_setDataSourceFD},
        {"_setVideoSurface",    "(Landroid/view/Surface;)V",                (void *) EMediaPlayer_setVideoSurface},
        {"_prepare",            "()V",                                      (void *) EMediaPlayer_prepare},
        {"_prepareAsync",       "()V",                                      (void *) EMediaPlayer_prepareAsync},
        {"_start",              "()V",                                      (void *) EMediaPlayer_start},
        {"_stop",               "()V",                                      (void *) EMediaPlayer_stop},
        {"_resume",             "()V",                                      (void *) EMediaPlayer_resume},
        {"_getRotate",          "()I",                                      (void *) EMediaPlayer_getRotate},
        {"_getVideoWidth",      "()I",                                      (void *) EMediaPlayer_getVideoWidth},
        {"_getVideoHeight",     "()I",                                      (void *) EMediaPlayer_getVideoHeight},
        {"_seekTo",             "(F)V",                                     (void *) EMediaPlayer_seekTo},
        {"_pause",              "()V",                                      (void *) EMediaPlayer_pause},
        {"_isPlaying",          "()Z",                                      (void *) EMediaPlayer_isPlaying},
        {"_getCurrentPosition", "()J",                                      (void *) EMediaPlayer_getCurrentPosition},
        {"_getDuration",        "()J",                                      (void *) EMediaPlayer_getDuration},
        {"_release",            "()V",                                      (void *) EMediaPlayer_release},
        {"_reset",              "()V",                                      (void *) EMediaPlayer_reset},
        {"_setLooping",         "(Z)V",                                     (void *) EMediaPlayer_setLooping},
        {"_isLooping",          "()Z",                                      (void *) EMediaPlayer_isLooping},
        {"_setVolume",          "(FF)V",                                    (void *) EMediaPlayer_setVolume},
        {"_setMute",            "(Z)V",                                     (void *) EMediaPlayer_setMute},
        {"_setRate",            "(F)V",                                     (void *) EMediaPlayer_setRate},
        {"_setPitch",           "(F)V",                                     (void *) EMediaPlayer_setPitch},
        {"native_init",         "()V",                                      (void *) EMediaPlayer_init},
        {"native_setup",        "(Ljava/lang/Object;)V",                    (void *) EMediaPlayer_setup},
        {"native_finalize",     "()V",                                      (void *) EMediaPlayer_finalize},
        {"_setOption",          "(ILjava/lang/String;Ljava/lang/String;)V", (void *) EMediaPlayer_setOption},
        {"_setOption",          "(ILjava/lang/String;J)V",                  (void *) EMediaPlayer_setOptionLong},
        {"_surfaceChange",      "(II)V",                                    (void *) EMediaPlayer_surfaceChange},
        {"_changeFilter",       "(ILjava/lang/String;)V",                   (void *)EMediaPlayer_changeFilter},
        {"_changeFilter",       "(II)V",                                    (void *)EMediaPlayer_changeFilterById},
        {"_setWatermark",       "([BIIIFI)V",                                 (void *)EMediaPlayer_setWatermark},
};

// 注册EMediaPlayer的Native方法
static int register_com_eplayer_EasyMediaPlayer(JNIEnv *env) {
    int numMethods = (sizeof(gMethods) / sizeof((gMethods)[0]));
    jclass clazz = env->FindClass(CLASS_NAME);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'", CLASS_NAME);
        return JNI_ERR;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("Native registration unable to find class '%s'", CLASS_NAME);
        return JNI_ERR;
    }
    env->DeleteLocalRef(clazz);

    return JNI_OK;
}

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    av_jni_set_java_vm(vm, NULL);
    javaVM = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    if (register_com_eplayer_EasyMediaPlayer(env) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_4;
}