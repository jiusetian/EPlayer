//
// Created by Guns Roses on 2020/4/23.
//
#include <cstring>
#include "FilterState.h"

FilterState::FilterState() {}

FilterState::~FilterState() {}

void FilterState::setFilterType(GLint filterType) {
    mFilterType = filterType;
}

void FilterState::setFilterColor(GLfloat *filterColor) {
    //设置纹理渲染滤镜颜色
    memcpy(mFilterColor, filterColor, sizeof(mFilterColor));

}
