
#include <AndroidLog.h>
#include <CoordinateUtils.h>
#include "GLESDevice.h"

GLESDevice::GLESDevice() {
    mWindow = NULL;
    mSurfaceWidth = 0;
    mSurfaceHeight = 0;
    eglSurface = EGL_NO_SURFACE;
    eglHelper = new EglHelper();

    mHaveEGLSurface = false;
    mHaveEGlContext = false;
    mHasSurface = false;

    mVideoTexture = (Texture *) malloc(sizeof(Texture));
    memset(mVideoTexture, 0, sizeof(Texture));
    mRenderNode = NULL;
    resetVertices();
    resetTexVertices();
}

GLESDevice::~GLESDevice() {
    mMutex.lock();
    terminate();
    mMutex.unlock();
}

void GLESDevice::surfaceChanged(int width, int height) {
    LOGD("执行3");
    if(mRenderNode!=NULL){
        mRenderNode->surfaceChanged(width,height);
    }
}

void GLESDevice::surfaceCreated(ANativeWindow *window) {
    mMutex.lock();
    if (mWindow != NULL) {
        ANativeWindow_release(mWindow);
        mWindow = NULL;
        //重新设置Surface
        mSurfaceReset = true;
    }
    mWindow = window;
    if (mWindow != NULL) {
        //渲染区域的宽高
        mSurfaceWidth = ANativeWindow_getWidth(mWindow);
        mSurfaceHeight = ANativeWindow_getHeight(mWindow);
        //LOGD("c++的宽高%d，%d", mSurfaceWidth, mSurfaceHeight);
    }
    mHasSurface = true;
    mMutex.unlock();
}

void GLESDevice::terminate() {
    terminate(true);
}

void GLESDevice::terminate(bool releaseContext) {
    if (eglSurface != EGL_NO_SURFACE) {
        LOGD("销毁surface");
        eglHelper->destroySurface(eglSurface);
        eglSurface = EGL_NO_SURFACE;
        mHaveEGLSurface = false;
    }

    if (eglHelper->getEglContext() != EGL_NO_CONTEXT && releaseContext) {
        if (mRenderNode) {
            mRenderNode->destroy();
            delete mRenderNode;
        }
        eglHelper->release();
        mHaveEGlContext = false;
    }
}

/**
 * 主要作用：初始化纹理相关，如果没有创建egl上下文和纹理渲染区域，就创建它们，并且关联egl上下文
 * 如果没有创建着色器和纹理对象，也要创建它们；还要一些其他数据的初始化
 * @param width
 * @param height
 * @param format
 * @param blendMode
 * @param rotate
 */
void GLESDevice::onInitTexture(int width, int height, TextureFormat format, BlendMode blendMode,
                               int rotate) {
    mMutex.lock();

    // 创建EGLContext
    if (!mHaveEGlContext) {
        //在这个init方法中会调用到egl中的相关初始化操作，包括创建和初始化EGLDisplay、初始化图形上下文EGLContext
        mHaveEGlContext = eglHelper->init(FLAG_TRY_GLES3);
        LOGD("mHaveEGlContext = %d", mHaveEGlContext);
    }

    if (!mHaveEGlContext) {
        return;
    }
    // 重新设置Surface，兼容SurfaceHolder处理
    //Android端的Surface是否已经传递进来了，同时Surface要reset
    if (mHasSurface && mSurfaceReset) {
        terminate(false);
        mSurfaceReset = false;
    }

    // 创建/释放EGLSurface
    if (eglSurface == EGL_NO_SURFACE && mWindow != NULL) {
        //如果已经有Surface但是还没有eglSurface，则创建之
        if (mHasSurface && !mHaveEGLSurface) {
            eglSurface = eglHelper->createSurface(mWindow);
            if (eglSurface != EGL_NO_SURFACE) {
                mHaveEGLSurface = true;
                LOGD("mHaveEGLSurface = %d", mHaveEGLSurface);
            }
        }
    } else if (eglSurface != EGL_NO_SURFACE && mHaveEGLSurface) {
        // 处于SurfaceDestroyed状态，释放EGLSurface
        if (!mHasSurface) {
            terminate(false);
        }
    }

    // 计算帧的宽高，如果不相等，则需要重新计算缓冲区的大小
    if (mWindow != NULL && mSurfaceWidth != 0 && mSurfaceHeight != 0) {
        // 当视频宽高比和窗口宽高比例不一致时，调整缓冲区的大小
        if ((mSurfaceWidth / mSurfaceHeight) != (width / height)) {
            //让视口的宽高比适应视频的宽高比
            //mSurfaceHeight = mSurfaceWidth * height / width; //这个不注释会有问题
            int windowFormat = ANativeWindow_getFormat(mWindow);
            //设置缓冲区参数，把width和height设置成你要显示的图片的宽和高
            ANativeWindow_setBuffersGeometry(mWindow, 0, 0, windowFormat);
        }
    }

    mVideoTexture->rotate = rotate;
    //LOGE("帧的宽度%d",width)
    //视频帧的宽高
    mVideoTexture->frameWidth = width;
    mVideoTexture->frameHeight = height;
    //纹理高度
    mVideoTexture->height = height;
    mVideoTexture->width=width;
    mVideoTexture->format = format;
    mVideoTexture->blendMode = blendMode;
    mVideoTexture->direction = FLIP_NONE;
    //关联egl上下文
    eglHelper->makeCurrent(eglSurface);
    //第一次会初始化，这个初始化主要作用是，创建对应的着色器程序和对应的纹理对象
    if (mRenderNode == NULL) {
        mRenderNode = new InputRenderNode();
        if (mRenderNode != NULL) {
            mRenderNode->initFilter(mVideoTexture);
            //设置视口的宽高
            mRenderNode->surfaceChanged(mSurfaceWidth,mSurfaceHeight);
        }
    }
    mMutex.unlock();
}

/**
 * 更新YUV数据，这里主要分别更新了YUV三个纹理对象的数据，数据分别有YUV的data数据和各自的宽度值
 * @param yData
 * @param yPitch
 * @param uData
 * @param uPitch
 * @param vData
 * @param vPitch
 * @return
 */
int GLESDevice::onUpdateYUV(uint8_t *yData, int yPitch, uint8_t *uData, int uPitch, uint8_t *vData,int vPitch) {
    if (!mHaveEGlContext) {
        return -1;
    }
    mMutex.lock();
    //分别赋值YUV的宽度
    mVideoTexture->pitches[0] = yPitch;
    mVideoTexture->pitches[1] = uPitch;
    mVideoTexture->pitches[2] = vPitch;
    //分别赋值YUV的像素数据
    mVideoTexture->pixels[0] = yData;
    mVideoTexture->pixels[1] = uData;
    mVideoTexture->pixels[2] = vData;

    if (mRenderNode != NULL && eglSurface != EGL_NO_SURFACE) {
        eglHelper->makeCurrent(eglSurface);
        mRenderNode->uploadTexture(mVideoTexture);
    }
    //LOGE("纹理的宽度%d",yPitch)
    // 设置像素实际的宽度，即linesize的值
    mVideoTexture->width = yPitch;
    mMutex.unlock();
    return 0;
}

int GLESDevice::onUpdateARGB(uint8_t *rgba, int pitch) {
    if (!mHaveEGlContext) {
        return -1;
    }
    mMutex.lock();
    //ARRB模式
    mVideoTexture->pitches[0] = pitch; //linesize
    mVideoTexture->pixels[0] = rgba; //data

    if (mRenderNode != NULL && eglSurface != EGL_NO_SURFACE) {
        eglHelper->makeCurrent(eglSurface);
        mRenderNode->uploadTexture(mVideoTexture);
    }
    // LOGE("纹理的宽度%d",pitch/4);
    // 设置像素实际的宽度，即linesize的值，因为BGRA_8888中一个像素为4个字节，所以这里像素长度等于字节长度除以4
    mVideoTexture->width = pitch / 4;
    mMutex.unlock();
    return 0;
}

int GLESDevice::onRequestRender(bool flip) {
    if (!mHaveEGlContext) {
        return -1;
    }
    mMutex.lock();
    mVideoTexture->direction = flip ? FLIP_VERTICAL : FLIP_NONE;
    //LOGD("flip ? %d", flip);
    if (mRenderNode != NULL && eglSurface != EGL_NO_SURFACE) {
        eglHelper->makeCurrent(eglSurface);
        if (mSurfaceWidth != 0 && mSurfaceHeight != 0) {
            mRenderNode->setDisplaySize(mSurfaceWidth, mSurfaceHeight);
            //LOGE("window的宽高=%d，%d",ANativeWindow_getWidth(mWindow),ANativeWindow_getHeight(mWindow));
        }
        mRenderNode->drawFrame(mVideoTexture);
        eglHelper->swapBuffers(eglSurface);
    }
    mMutex.unlock();
    return 0;
}

void GLESDevice::resetVertices() {
    const float *verticesCoord = CoordinateUtils::getVertexCoordinates();
    for (int i = 0; i < 8; ++i) {
        vertices[i] = verticesCoord[i];
    }
}

void GLESDevice::resetTexVertices() {
    const float *vertices = CoordinateUtils::getTextureCoordinates(ROTATE_NONE);
    for (int i = 0; i < 8; ++i) {
        textureVertices[i] = vertices[i];
    }
}
