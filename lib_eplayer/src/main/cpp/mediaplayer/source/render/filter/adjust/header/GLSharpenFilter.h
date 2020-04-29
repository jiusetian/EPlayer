
#ifndef GLSHARPENFILTER_H
#define GLSHARPENFILTER_H

#include <GLIntensityFilter.h>

class GLSharpenFilter : public GLIntensityFilter {
public:
    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

protected:
    void onDrawBegin() override;

private:
    int widthFactorHandle;
    int heightFactorHandle;
};


#endif //GLSHARPENFILTER_H
