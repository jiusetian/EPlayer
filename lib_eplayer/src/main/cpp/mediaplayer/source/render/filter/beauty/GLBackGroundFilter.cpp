//
// Created by Guns Roses on 2020/4/30.
//
#include "GLBackGroundFilter.h"

const std::string backGroundFragmentShader = SHADER_TO_STRING(
        precision
        mediump float;
        uniform int iFilterType;
        uniform
        vec3 vFilterColor;

        uniform
        sampler2D inputTexture;
        varying
        vec2 textureCoordinate;

        void main() {
            //转换一下格式，texture2D是2.0的函数，3.0要用texture
            vec4 abgr = texture2D(inputTexture, textureCoordinate);
            vec4 tmpColor = abgr;
            //tmpColor.r = abgr.b;
            //tmpColor.b = abgr.r;
            gl_FragColor = tmpColor;

            if (iFilterType == 1) { // 灰度图
                float c = tmpColor.r * vFilterColor.r + tmpColor.g * vFilterColor.g + tmpColor.b * vFilterColor.b;
                gl_FragColor = vec4(c, c, c, tmpColor.a);
            } else if (iFilterType == 2) { // 冷暖色
                tmpColor.r = min(1.0, tmpColor.r + vFilterColor.r);
                tmpColor.g = min(1.0, tmpColor.g + vFilterColor.g);
                tmpColor.b = min(1.0, tmpColor.b + vFilterColor.b);
                gl_FragColor = tmpColor;
            } else if (iFilterType == 3) { // 模糊
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x - vFilterColor.r, textureCoordinate.y - vFilterColor.r));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x - vFilterColor.r, textureCoordinate.y + vFilterColor.r));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x + vFilterColor.r, textureCoordinate.y - vFilterColor.r));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x + vFilterColor.r, textureCoordinate.y + vFilterColor.r));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x - vFilterColor.g, textureCoordinate.y - vFilterColor.g));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x - vFilterColor.g, textureCoordinate.y + vFilterColor.g));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x + vFilterColor.g, textureCoordinate.y - vFilterColor.g));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x + vFilterColor.g, textureCoordinate.y + vFilterColor.g));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x - vFilterColor.b, textureCoordinate.y - vFilterColor.b));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x - vFilterColor.b, textureCoordinate.y + vFilterColor.b));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x + vFilterColor.b, textureCoordinate.y - vFilterColor.b));
                tmpColor += texture2D(inputTexture,
                                      vec2(textureCoordinate.x + vFilterColor.b, textureCoordinate.y + vFilterColor.b));
                tmpColor /= 13.0;
                gl_FragColor = tmpColor;
            } else { // 原色
                gl_FragColor = tmpColor;
            }
        }
);

GLBackGroundFilter::GLBackGroundFilter(GLint filterType, GLfloat *filterColor) {
    this->filterType = filterType;
    memcpy(this->filterColor, filterColor, sizeof(this->filterColor));
}

void GLBackGroundFilter::initProgram() {
    initProgram(kDefaultVertexShader.c_str(), backGroundFragmentShader.c_str());
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


