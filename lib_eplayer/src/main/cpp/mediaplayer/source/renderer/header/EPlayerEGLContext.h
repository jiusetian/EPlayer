
#ifndef EPLAYER_EPLAYEREGLCONTEXT_H
#define EPLAYER_EPLAYEREGLCONTEXT_H

#include <mutex>

#if defined(__ANDROID__)

#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#endif

class EPlayerEGLContext {

public:
    static EPlayerEGLContext *getInstance();

    void destory();

    EGLContext getContext();

private:
    EPlayerEGLContext();

    virtual ~EPlayerEGLContext();

    bool init(int flags);

    void release();

    EGLConfig getConfig(int flags, int version);

    void checkEglError(const char *msg);

    static EPlayerEGLContext *instance;
    static std::mutex mutex;

    EGLContext eglContext;
    EGLDisplay eglDisplay;
};

#endif //EPLAYER_EPLAYEREGLCONTEXT_H
