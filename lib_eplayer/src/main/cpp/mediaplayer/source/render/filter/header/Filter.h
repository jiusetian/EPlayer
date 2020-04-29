

#ifndef FILTER_H
#define FILTER_H

#include "render/common/header/macros.h"

#include "FrameBuffer.h"
#include "FilterManager.h"

#include "render/common/header/EglHelper.h"
#include "render/common/header/OpenGLUtils.h"
#include "render/common/header/vecmath.h"
#include "render/common/header/CoordinateUtils.h"

// 基础滤镜
#include "GLGaussianBlurFilter.h"
#include "GLGroupFilter.h"

// 输入滤镜
#include "GLInputFilter.h"
#include "GLInputABGRFilter.h"
#include "GLInputYUV420PFilter.h"
#include "GLOESInputFilter.h"

// 美颜滤镜
#include "GLBeautyFilter.h"

// 滤镜特效
#include "GLEffectGlitterWhiteFilter.h"
#include "GLEffectIllusionFilter.h"
#include "GLEffectScaleFilter.h"
#include "GLEffectShakeFilter.h"
#include "GLEffectSoulStuffFilter.h"

// 分屏特效滤镜
#include "GLFrameBlackWhiteThreeFilter.h"
#include "GLFrameBlurFilter.h"
#include "GLFrameTwoFilter.h"
#include "GLFrameThreeFilter.h"
#include "GLFrameFourFilter.h"
#include "GLFrameSixFilter.h"
#include "GLFrameNineFilter.h"

#endif //FILTER_H
