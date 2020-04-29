
#ifndef GLBEAUTYBLURFILTER_H
#define GLBEAUTYBLURFILTER_H


#include <GLGaussianBlurFilter.h>

/**
 * 美颜用的高斯模糊滤镜
 */
class GLBeautyBlurFilter : public GLGaussianBlurFilter {
public:
    GLBeautyBlurFilter();

    GLBeautyBlurFilter(const char *vertexShader, const char *fragmentShader);
};


#endif //GLBEAUTYBLURFILTER_H
