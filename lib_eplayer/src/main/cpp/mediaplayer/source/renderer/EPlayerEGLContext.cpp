
#include "renderer/header/EPlayerEGLContext.h"
#include "AndroidLog.h"
#include "renderer/helper/EglHelper.h"

EPlayerEGLContext *EPlayerEGLContext::instance;
//std::mutex 是C++11 中最基本的互斥量
std::mutex EPlayerEGLContext::mutex;

EPlayerEGLContext::EPlayerEGLContext() {
    eglDisplay = EGL_NO_DISPLAY;
    eglContext = EGL_NO_CONTEXT;
    init(FLAG_TRY_GLES3);
}

EPlayerEGLContext::~EPlayerEGLContext() {

}

EPlayerEGLContext *EPlayerEGLContext::getInstance() {
    if (!instance) {
        std::unique_lock<std::mutex> lock(mutex);
        if (!instance) {
            instance = new(std::nothrow) EPlayerEGLContext();
        }
    }
    return instance;
}

bool EPlayerEGLContext::init(int flags) {
    if (eglDisplay != EGL_NO_DISPLAY) {
        LOGE("EGL already set up");
        return false;
    }

    // 获取EGLDisplay
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        LOGE("unable to get EGLDisplay.\n");
        return false;
    }

    // 初始化mEGLDisplay
    if (!eglInitialize(eglDisplay, 0, 0)) {
        eglDisplay = EGL_NO_DISPLAY;
        LOGE("unable to initialize EGLDisplay.");
        return false;
    }

    // 判断是否尝试使用GLES3
    if ((flags & FLAG_TRY_GLES3) != 0) {
        EGLConfig config = getConfig(flags, 3);
        if (config != NULL) {
            int attrib3_list[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
            //创建渲染上下文
            EGLContext context = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, attrib3_list);
            checkEglError("eglCreateContext");
            if (eglGetError() == EGL_SUCCESS) {
                eglContext = context;
            }
        }
    }

    // 判断如果GLES3的EGLContext没有获取到，则尝试使用GLES2
    if (eglContext == EGL_NO_CONTEXT) {
        //获取配置属性
        EGLConfig config = getConfig(flags, 2);
        int attrib2_list[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        //创建图形上下文
        EGLContext context = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, attrib2_list);
        checkEglError("eglCreateContext");
        if (eglGetError() == EGL_SUCCESS) {
            eglContext = context;
        }
    }

    int values[1] = {0};
    //查询context的参数
    eglQueryContext(eglDisplay, eglContext, EGL_CONTEXT_CLIENT_VERSION, values);
    LOGD("EGLContext created, client version %d", values[0]);
    return true;
}


EGLConfig EPlayerEGLContext::getConfig(int flags, int version) {
    int renderableType = EGL_OPENGL_ES2_BIT;
    if (version >= 3) {
        renderableType |= EGL_OPENGL_ES3_BIT_KHR;
    }
    int attribList[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            //EGL_DEPTH_SIZE, 16,
            //EGL_STENCIL_SIZE, 8,
            EGL_RENDERABLE_TYPE, renderableType,
            EGL_NONE, 0,      // placeholder for recordable [@-3]
            EGL_NONE
    };
    int length = sizeof(attribList) / sizeof(attribList[0]);
    if ((flags & FLAG_RECORDABLE) != 0) {
        //告诉EGL它创建的surface必须和视频编解码器兼容。没有这个标志，EGL可能会使用一个MediaCodec不能理解的Buffer
        attribList[length - 3] = EGL_RECORDABLE_ANDROID;
        attribList[length - 2] = 1;
    }
    EGLConfig configs = NULL;
    int numConfigs;
    //推荐最佳匹配
    if (!eglChooseConfig(eglDisplay, attribList, &configs, 1, &numConfigs)) {
        LOGW("unable to find RGB8888 / %d  EGLConfig", version);
        return NULL;
    }
    return configs;
}


void EPlayerEGLContext::destory() {
    if (instance) {
        std::unique_lock<std::mutex> lock(mutex);
        if (!instance) {
            delete instance;
            instance = nullptr;
        }
    }
}

void EPlayerEGLContext::checkEglError(const char *msg) {
    int error;
    if ((error = eglGetError()) != EGL_SUCCESS) {
        LOGE("%s: EGL error: %x", msg, error);
    }
}

EGLContext EPlayerEGLContext::getContext() {
    return eglContext;
}