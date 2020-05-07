//
// Created by Guns Roses on 2020/5/7.
//
#include "GLEffectPipFilter.h"

GLEffectPipFilter::GLEffectPipFilter() {}

void GLEffectPipFilter::initProgram() {
    initProgram(kDefaultVertexShader.c_str(), PIPFragmentShader.c_str());
}

void GLEffectPipFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader, fragmentShader);
}

