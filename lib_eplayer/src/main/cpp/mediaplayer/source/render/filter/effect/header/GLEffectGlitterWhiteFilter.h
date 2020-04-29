
#ifndef GLEFFECTGLITTERWHITEFILTER_H
#define GLEFFECTGLITTERWHITEFILTER_H


#include <GLFilter.h>

class GLEffectGlitterWhiteFilter : public GLFilter {
public:
    GLEffectGlitterWhiteFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    void setTimeStamp(double timeStamp) override;

protected:
    void onDrawBegin() override;

private:
    int colorHandle;
    float color;
};


#endif //GLEFFECTGLITTERWHITEFILTER_H
