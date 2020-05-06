//
// Created by Guns Roses on 2020/4/30.
//
#include "GLBackGroundFilter.h"


GLBackGroundFilter::GLBackGroundFilter(GLint filterType, GLfloat *filterColor) {
    this->filterType = filterType;
    memcpy(this->filterColor, filterColor, sizeof(this->filterColor));
}

void GLBackGroundFilter::initProgram() {
    initProgram(kDefaultVertexShader.c_str(), BackGroundFragmentShader.c_str());
}

void GLBackGroundFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader,fragmentShader);
    if (isInitialized()) {
        filterTypeHandle = glGetUniformLocation(programHandle, "iFilterType");
        filterColorHandle = glGetUniformLocation(programHandle, "vFilterColor");
    }
}

void GLBackGroundFilter::onDrawBegin() {
    glUniform1i(filterTypeHandle, filterType);
    glUniform3fv(filterColorHandle, 1, filterColor);
}


