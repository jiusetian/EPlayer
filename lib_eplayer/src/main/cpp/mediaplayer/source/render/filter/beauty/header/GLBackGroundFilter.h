//
// Created by Guns Roses on 2020/4/30.
//

#ifndef EPLAYER_GLBACKGROUNDFILTER_H
#define EPLAYER_GLBACKGROUNDFILTER_H

#include "GLFilter.h"

class GLBackGroundFilter : public GLFilter {

public:

    GLBackGroundFilter(GLint filterType, GLfloat filterColor[3]);

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

protected:
    void onDrawBegin() override;

private:
    GLint filterTypeHandle; // 类型句柄
    GLint filterColorHandle; // 颜色句柄

    GLint filterType;       // 颜色类型
    GLfloat filterColor[3]; // 颜色
};

// 背景颜色片元着色器
const std::string BackGroundFragmentShader = SHADER_TO_STRING(

        precision mediump float;
        uniform int iFilterType;
        uniform vec3 vFilterColor;
        uniform sampler2D inputTexture;
        varying vec2 textureCoordinate;

        void main() {
            //转换一下格式，texture2D是2.0的函数，3.0要用texture
            vec4 argb = texture2D(inputTexture, textureCoordinate);
            vec4 tmpColor = argb;

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


#endif //EPLAYER_GLBACKGROUNDFILTER_H
