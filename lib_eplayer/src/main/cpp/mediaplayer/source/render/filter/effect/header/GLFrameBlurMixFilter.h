
#ifndef GLFRAMEBLURMIXFILTER_H
#define GLFRAMEBLURMIXFILTER_H

#include <GLFilter.h>

/**
 * 分屏模糊混合
 */
class GLFrameBlurMixFilter : public GLFilter {
public:
    GLFrameBlurMixFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    void setBlurTexture(int blurTexture);

protected:
    void bindTexture(GLuint texture) override;

    void onDrawBegin() override;

private:
    int blurTextureHandle;
    int blurOffsetYHandle;
    int scaleHandle;

    int blurTexture;
    float blurOffsetY;
    float scale;
};


#endif //GLFRAMEBLURMIXFILTER_H
