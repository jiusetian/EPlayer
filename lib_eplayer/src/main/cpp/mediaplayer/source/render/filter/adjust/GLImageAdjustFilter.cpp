

#include "header/GLImageAdjustFilter.h"
#include "header/GLBrightnessFilter.h"
#include "header/GLContrastFilter.h"
#include "header/GLExposureFilter.h"
#include "header/GLHueFilter.h"
#include "header/GLSaturationFilter.h"
#include "header/GLSharpenFilter.h"

GLImageAdjustFilter::GLImageAdjustFilter() {
    addFilter(new GLBrightnessFilter());
    addFilter(new GLContrastFilter());
    addFilter(new GLExposureFilter());
    addFilter(new GLHueFilter());
    addFilter(new GLSaturationFilter());
    addFilter(new GLSharpenFilter());
}

void GLImageAdjustFilter::setAdjustIntensity(const float *adjust) {
    for (int i = 0; i < 6; ++i) {
        filterList[i]->setIntensity(adjust[i]);
    }
}
