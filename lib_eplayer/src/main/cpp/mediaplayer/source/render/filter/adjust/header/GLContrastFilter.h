

#ifndef GLCONTRASTFILTER_H
#define GLCONTRASTFILTER_H

#include <GLIntensityFilter.h>

/**
 * 对比度滤镜
 */
class GLContrastFilter : public GLIntensityFilter {

public:
    void initProgram() override;
};


#endif //GLCONTRASTFILTER_H
