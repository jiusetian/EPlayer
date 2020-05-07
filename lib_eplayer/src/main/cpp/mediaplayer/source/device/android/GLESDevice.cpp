
#include <AndroidLog.h>
#include "CoordinateUtils.h"
#include "DisplayRenderNode.h"
#include "GLESDevice.h"
#include "GLWatermarkFilter.h"


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

    nodeList = new RenderNodeList();
    nodeList->addNode(new DisplayRenderNode());     // 添加显示渲染的结点
    memset(&filterInfo, 0, sizeof(FilterInfo));
    filterInfo.type = NODE_NONE;
    filterInfo.name = nullptr;
    filterInfo.id = -1;
    filterChange = true;

    resetVertices();
    resetTexVertices();
}

GLESDevice::~GLESDevice() {
    mMutex.lock();
    terminate();
    mMutex.unlock();
}

void GLESDevice::surfaceChanged(int width, int height) {
    mMutex.lock();
    mSurfaceWidth = width;
    mSurfaceHeight = height;
    if (nodeList != nullptr && nodeList->findNode(NODE_DISPLAY) != nullptr) {
        // 对象在指针强转要注意，比如这里的findNode返回的RenderNode *也是可以强转为GLOutFilter *的，但是不能使用，编译切没有问题
        GLOutFilter *glOutFilter = (GLOutFilter *) nodeList->findNode(NODE_DISPLAY)->glFilter;
        glOutFilter->nativeSurfaceChanged(width, height);
    }
    mMutex.unlock();
}

void GLESDevice::setTimeStamp(double timeStamp) {
    mMutex.lock();
    if (nodeList) {
        nodeList->setTimeStamp(timeStamp);
    }
    mMutex.unlock();
}

void GLESDevice::surfaceCreated(ANativeWindow *window) {
    mMutex.lock();
    if (mWindow != NULL) {
        ANativeWindow_release(mWindow);
        mWindow = NULL;
        // 重置window则需要重置Surface
        mSurfaceReset = true;
    }
    mWindow = window;
    if (mWindow != NULL) {
        // 渲染区域的宽高
        mSurfaceWidth = ANativeWindow_getWidth(mWindow);
        mSurfaceHeight = ANativeWindow_getHeight(mWindow);
    }
    mHasSurface = true;
    mMutex.unlock();
}

void GLESDevice::terminate() {
    terminate(true);
}

void GLESDevice::terminate(bool releaseContext) {
    // 销毁Surface
    if (eglSurface != EGL_NO_SURFACE) {
        eglHelper->destroySurface(eglSurface);
        eglSurface = EGL_NO_SURFACE;
        mHaveEGLSurface = false;
    }

    // 销毁egl上下文
    if (eglHelper->getEglContext() != EGL_NO_CONTEXT && releaseContext) {
        if (mRenderNode) {
            mRenderNode->destroy();
            delete mRenderNode;
        }
        eglHelper->release();
        mHaveEGlContext = false;
    }
}

void GLESDevice::changeFilter(RenderNodeType type, const char *filterName) {
    mMutex.lock();
    filterInfo.type = type;
    filterInfo.name = av_strdup(filterName);
    filterInfo.id = -1;
    filterChange = true;
    mMutex.unlock();
}

void GLESDevice::changeFilter(RenderNodeType type, const int id) {

    mMutex.lock();
    filterInfo.type = type;
    if (filterInfo.name) {
        av_freep(&filterInfo.name);
        filterInfo.name = nullptr;
    }
    filterInfo.name = nullptr;
    filterInfo.id = id;
    filterChange = true;
    mMutex.unlock();
}

void GLESDevice::setWatermark(uint8_t *watermarkPixel, size_t length, GLint width, GLint height, GLfloat scale,
                              GLint location) {

    mMutex.lock();
    RenderNode *node = nodeList->findNode(NODE_STICKERS);
    if (!node) {
        node = new RenderNode(NODE_STICKERS);
    }
    // 创建水印滤镜
    GLWatermarkFilter *watermarkFilter = new GLWatermarkFilter();
    watermarkFilter->setTextureSize(mVideoTexture->width,mVideoTexture->height);
    watermarkFilter->setDisplaySize(mSurfaceWidth,mSurfaceHeight);
    watermarkFilter->setWatermark(watermarkPixel, length, width, height, scale, location);
    node->changeFilter(watermarkFilter);
    nodeList->addNode(node); // 添加水印节点
    // 要在onInitTexture中初始化水印filter，可能原因是egl属性的创建(比如着色器program，elgsurface等)和使用需要在同一个线程中完成
    filterInfo.type = NODE_NONE;
    filterInfo.name = nullptr;
    filterInfo.id = -1;
    filterChange = true;
    mMutex.unlock();
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
        // 在这个init方法中会调用到egl中的相关初始化操作，包括创建和初始化EGLDisplay、初始化图形上下文EGLContext
        mHaveEGlContext = eglHelper->init(FLAG_TRY_GLES3);
        LOGD("mHaveEGlContext = %d", mHaveEGlContext);
    }

    if (!mHaveEGlContext) {
        return;
    }
    // 是否需要重置Surface，兼容SurfaceHolder处理
    if (mHasSurface && mSurfaceReset) {
        terminate(false);
        mSurfaceReset = false;
    }

    // 创建EGLSurface
    if (eglSurface == EGL_NO_SURFACE && mWindow != NULL) {
        // 如果已经有Surface但是还没有eglSurface，则创建之
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
            // 让视口的宽高比适应视频的宽高比
            // mSurfaceHeight = mSurfaceWidth * height / width; // 这个不注释会有问题
            int windowFormat = ANativeWindow_getFormat(mWindow);
            // 设置缓冲区参数，把width和height设置成你要显示的图片的宽和高
            ANativeWindow_setBuffersGeometry(mWindow, 0, 0, windowFormat);
            // ANativeWindow_setBuffersGeometry(mWindow, mSurfaceWidth, mSurfaceHeight, windowFormat);
        }
    }

    mVideoTexture->rotate = rotate;
    // LOGE("帧的宽度%d",width)
    // 视频帧的宽高
    mVideoTexture->frameWidth = width;
    mVideoTexture->frameHeight = height;
    // 纹理高度
    mVideoTexture->height = height;
    mVideoTexture->width = width;
    mVideoTexture->format = format;
    mVideoTexture->blendMode = blendMode;
    mVideoTexture->direction = FLIP_NONE;
    // 关联egl上下文
    eglHelper->makeCurrent(eglSurface);

    // 第一次会初始化，这个初始化主要作用是，创建对应的着色器程序和对应的纹理对象
    if (mRenderNode == NULL) {
        // 初始化输入节点
        mRenderNode = new InputRenderNode();
        if (mRenderNode != NULL) {
            // 输入节点的初始化
            mRenderNode->initFilter(mVideoTexture);

            // 创建一个FBO给渲染节点
            FrameBuffer *frameBuffer = new FrameBuffer(width, height);
            // 初始化主要是创建一个帧缓冲区FBO并挂载一个颜色纹理
            frameBuffer->init();
            mRenderNode->setFrameBuffer(frameBuffer);

            // 设置所有节点的视口大小
            if (mSurfaceWidth != 0 && mSurfaceHeight != 0) {
                nodeList->setDisplaySize(mSurfaceWidth, mSurfaceHeight);
            }
        }
    }
    // 如果改变了滤镜效果的渲染，则在节点链表中增加或更改为当前的滤镜
    if (filterChange) {
        // 改变滤镜
        nodeList->changeFilter(filterInfo.type, FilterManager::getInstance()->getFilter(&filterInfo));
        filterChange = false;
        // 节点的视口大小
        if (mSurfaceWidth != 0 && mSurfaceHeight != 0) {
            nodeList->setDisplaySize(mSurfaceWidth, mSurfaceHeight);
        }
        // 设置纹理大小，根据纹理大小初始化FBO
        nodeList->setTextureSize(width, height);
        // 节点初始化，主要是filter的初始化
        nodeList->init(); //filter初始化了以后才能用
    }
    mMutex.unlock();
}

/**
 * 更新YUV数据，分别更新YUV三个纹理对象的数据，数据分别有YUV的data数据和各自的宽度值
 * @param yData
 * @param yPitch
 * @param uData
 * @param uPitch
 * @param vData
 * @param vPitch
 * @return
 */
int GLESDevice::onUpdateYUV(uint8_t *yData, int yPitch, uint8_t *uData, int uPitch, uint8_t *vData, int vPitch) {
    if (!mHaveEGlContext) {
        return -1;
    }
    mMutex.lock();
    // 分别赋值YUV的宽度
    mVideoTexture->pitches[0] = yPitch;
    mVideoTexture->pitches[1] = uPitch;
    mVideoTexture->pitches[2] = vPitch;
    // 分别赋值YUV的像素数据
    mVideoTexture->pixels[0] = yData;
    mVideoTexture->pixels[1] = uData;
    mVideoTexture->pixels[2] = vData;

    if (mRenderNode != NULL && eglSurface != EGL_NO_SURFACE) {
        eglHelper->makeCurrent(eglSurface);
        mRenderNode->uploadTexture(mVideoTexture);
    }
    // LOGE("纹理的宽度%d",yPitch)
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
    // ARGB模式
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
    // LOGD("flip ? %d", flip);
    if (mRenderNode != NULL && eglSurface != EGL_NO_SURFACE) {
        eglHelper->makeCurrent(eglSurface);

        // 第一步是输入节点的渲染，mRenderNode就是输入节点，输入节点也有一个FBO，此时将渲染结果保存到FBO中
        int texture = mRenderNode->drawFrameBuffer(mVideoTexture);

//        if (mSurfaceWidth != 0 && mSurfaceHeight != 0) {
//            // 设置显示窗口的大小
//            nodeList->setDisplaySize(mSurfaceWidth, mSurfaceHeight);
//            // LOGE("window的宽高=%d，%d",ANativeWindow_getWidth(mWindow),ANativeWindow_getHeight(mWindow));
//        }
        // 渲染纹理，直接渲染到系统默认的帧缓冲区，而不是自定义的帧缓冲区FBO
        // mRenderNode->drawFrame(mVideoTexture);
        // 从渲染节点链表中的节点依次对纹理数据进行处理，并最后显示
        nodeList->drawFrame(texture, vertices, textureVertices);
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
    // 这个纹理坐标是FBO的纹理坐标，以左下角为原点
    const float *vertices = CoordinateUtils::getTextureCoordinates(ROTATE_NONE);
    for (int i = 0; i < 8; ++i) {
        textureVertices[i] = vertices[i];
    }
}
