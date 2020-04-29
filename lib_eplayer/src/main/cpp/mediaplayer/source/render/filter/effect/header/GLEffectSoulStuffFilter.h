

#ifndef GLEFFECTSOULSTUFFFILTER_H
#define GLEFFECTSOULSTUFFFILTER_H

#include <GLFilter.h>

class GLEffectSoulStuffFilter : public GLFilter {
public:
    GLEffectSoulStuffFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    void setTimeStamp(double timeStamp) override;

protected:
    void onDrawBegin() override;

private:
    int scaleHandle;
    float scale;
    float offset;
};


#endif //GLEFFECTSOULSTUFFFILTER_H
