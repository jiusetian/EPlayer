
#include <render/common/header/OpenGLUtils.h>
#include "GLOutFilter.h"


GLOutFilter::GLOutFilter() {}

GLOutFilter::~GLOutFilter() {}

// 在Surface尺寸发生改变的时候调用，主要用于适配播放的宽高比，主要通过计算矩阵来实现
void GLOutFilter::nativeSurfaceChanged(int width, int height) {
    displayWidth = width;
    displayHeight = height;
    // 如果视频宽高不为0，则改变矩阵
    if (textureWidth != 0 && textureHeight != 0) {
        LOGD("改变矩阵1");
        v_mat4 = OpenGLUtils::caculateVideoFitMat4(textureWidth, textureHeight, displayWidth, displayHeight);
    }
}

void GLOutFilter::onDrawBegin() {
    // 设置矩阵的值
    glUniformMatrix4fv(matrixHandle, 1, GL_FALSE, glm::value_ptr(v_mat4));
}

void GLOutFilter::initProgram() {
    if (!isInitialized()) {
        initProgram(kOutFilterVertexShader.c_str(), kDefaultFragmentShader.c_str());
    }
}

void GLOutFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader,fragmentShader);

    // 如果视频和Surface都不为0，则初始化屏幕适配矩阵
    if (textureWidth != 0 && textureHeight != 0 && displayWidth != 0 && displayHeight != 0) {
        LOGD("改变矩阵2");
        v_mat4 = OpenGLUtils::caculateVideoFitMat4(textureWidth, textureHeight, displayWidth, displayHeight);
    }

    if(isInitialized()){
        // 顶点矩阵
        matrixHandle = glGetUniformLocation(programHandle, "uMatrix");
    }
}
