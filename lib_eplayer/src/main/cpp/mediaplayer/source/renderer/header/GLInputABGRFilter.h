
#ifndef EPLAYER_GLINPUTABGRFILTER_H
#define EPLAYER_GLINPUTABGRFILTER_H

#include "GLInputFilter.h"
/**
 * BGRA输入滤镜
 */
class GLInputABGRFilter : public GLInputFilter {
public:
    GLInputABGRFilter();

    virtual ~GLInputABGRFilter();
    //override是重写的意思，代表这个方法本身是虚函数，虚函数是为了C++多态的实现，当用指向子类对象的父类指针调用虚函数的时候，调用的是
    //子类重写的函数，而不是父类的被重写函数，但是如果不标明虚函数的话，就会调用父类的函数
    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    void bindUniforms() override ;

    GLboolean renderTexture(Texture *texture, float *vertices, float *textureVertices) override;

    GLboolean uploadTexture(Texture *texture) override;

private:
    GLint mFilterTypeLoc;
    GLint mFilterColorLoc;
    //滤镜类型
    GLint mFilterType;
    //滤镜颜色
    GLfloat mFilterColor[3];


};


#endif //EPLAYER_GLINPUTABGRFILTER_H
