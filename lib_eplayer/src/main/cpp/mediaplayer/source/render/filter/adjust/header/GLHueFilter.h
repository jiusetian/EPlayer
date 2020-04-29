

#ifndef GLHUEFILTER_H
#define GLHUEFILTER_H

#include <GLIntensityFilter.h>

/**
 * 色调调节滤镜
 */
class GLHueFilter : public GLIntensityFilter {
public:
    void initProgram() override;
};


#endif //GLHUEFILTER_H
