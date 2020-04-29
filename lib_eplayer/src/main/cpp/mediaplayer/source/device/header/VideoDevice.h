
#ifndef EPLAYER_VIDEODEVICE_H
#define EPLAYER_VIDEODEVICE_H

#include <render/filter/input/header/GLInputFilter.h>
#include "PlayerState.h"


class VideoDevice {
public:
    VideoDevice();

    virtual ~VideoDevice();

    virtual void terminate();

    // 设置时间戳
    virtual void setTimeStamp(double timeStamp);

    // 初始化视频纹理宽高
    virtual void onInitTexture(int width, int height, TextureFormat format, BlendMode blendMode, int rotate = 0);

    // 更新YUV数据
    virtual int onUpdateYUV(uint8_t *yData, int yPitch,
                            uint8_t *uData, int uPitch,
                            uint8_t *vData, int vPitch);

    // 更新ARGB数据
    virtual int onUpdateARGB(uint8_t *rgba, int pitch);

    // 请求渲染
    virtual int onRequestRender(bool flip);

};
#endif //EPLAYER_VIDEODEVICE_H
