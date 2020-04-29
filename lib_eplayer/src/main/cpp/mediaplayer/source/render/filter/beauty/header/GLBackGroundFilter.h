//
// Created by Guns Roses on 2020/4/30.
//

#ifndef EPLAYER_GLBACKGROUNDFILTER_H
#define EPLAYER_GLBACKGROUNDFILTER_H

#include "GLFilter.h"

class GLBackGroundFilter : public GLFilter{

public:

    GLBackGroundFilter(GLint filterType,GLfloat filterColor[3]);

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


#endif //EPLAYER_GLBACKGROUNDFILTER_H
