//
// Created by Guns Roses on 2020/5/8.
//

#include "GLEffectRotateCircleFilter.h"

GLEffectRotateCircleFilter::GLEffectRotateCircleFilter() {}

void GLEffectRotateCircleFilter::initProgram() {
    initProgram(kDefaultVertexShader.c_str(), RotateCircleFragmentShader.c_str());
}

void GLEffectRotateCircleFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader, fragmentShader);
    if (isInitialized()) {
        LOGD("转圜初始化");
        texSizeHandle = glGetUniformLocation(programHandle, "texSize");
        offsetHandle = glGetUniformLocation(programHandle, "u_offset");
    }
}

void GLEffectRotateCircleFilter::onDrawBegin() {
    GLFilter::onDrawBegin();
    if (isInitialized()) {
        glUniform2fv(texSizeHandle, 1, texSize.ptr());
        glUniform1f(offsetHandle, offset);
    }
}

void GLEffectRotateCircleFilter::setTextureSize(int width, int height) {
    GLFilter::setTextureSize(width, height);
    texSize = Vector2((float) width, (float) height);
    LOGD("转圜设置大小");
}

void GLEffectRotateCircleFilter::setDisplaySize(int width, int height) {
    GLFilter::setDisplaySize(width,height);
    //texSize = Vector2((float) width, (float) height);
}

void GLEffectRotateCircleFilter::setTimeStamp(double timeStamp) {
    GLFilter::setTimeStamp(timeStamp);
    //LOGD("时间戳：%lf",timeStamp);
    offset = (sin(timeStamp * PI / 20) + 1.0f) / 2.0f;
}
