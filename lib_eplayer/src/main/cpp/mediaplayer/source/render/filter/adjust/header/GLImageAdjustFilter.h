

#ifndef GLIMAGEADJUSTFILTER_H
#define GLIMAGEADJUSTFILTER_H


#include <GLGroupFilter.h>

class GLImageAdjustFilter : public GLGroupFilter {
public:
    GLImageAdjustFilter();

    void setAdjustIntensity(const float *adjust);
};


#endif //GLIMAGEADJUSTFILTER_H
