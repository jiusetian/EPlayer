
#ifndef GLEFFECTSCALEFILTER_H
#define GLEFFECTSCALEFILTER_H

#include <GLFilter.h>

class GLEffectScaleFilter : public GLFilter {
public:
    GLEffectScaleFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    void setTimeStamp(double timeStamp) override;

protected:
    void onDrawBegin() override;

private:
    int scaleHandle;
    bool plus;
    float scale;
    float offset;
};


#endif //GLEFFECTSCALEFILTER_H
