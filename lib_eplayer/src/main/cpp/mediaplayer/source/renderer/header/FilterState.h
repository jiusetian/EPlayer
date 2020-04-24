//
// Created by Guns Roses on 2020/4/23.
//

#ifndef EPLAYER_FILTERSTATE_H
#define EPLAYER_FILTERSTATE_H

#include <GLES2/gl2.h>

//滤镜的相关状态
class FilterState{
private:
    GLint mFilterType;       //滤镜类型
    GLfloat mFilterColor[3]; //滤镜颜色
    bool isTwoScreen;        //是否双屏

public:
    FilterState();

    virtual ~FilterState();

    //设置滤镜类型
    void setFilterType(GLint filterType);
    GLint getFilterType() const{ return mFilterType;}

    //设置滤镜颜色
    void setFilterColor(GLfloat *filterColor);
    GLfloat *getFilterColor()  { return mFilterColor;}

    //设置是否双屏
    void setIsTwoScreen(bool isTwoScreen);
    bool istwoScreen() const{ return isTwoScreen;}

};
#endif //EPLAYER_FILTERSTATE_H
