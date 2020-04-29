

#ifndef GLEXPOSUREFILTER_H
#define GLEXPOSUREFILTER_H

#include <GLIntensityFilter.h>

/**
 * 曝光滤镜
 */
class GLExposureFilter : public GLIntensityFilter {

public:
    void initProgram() override;
};


#endif //GLEXPOSUREFILTER_H
