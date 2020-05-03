
#include "GLEffectSoulStuffFilter.h"
#include <math.h>
#include <AndroidLog.h>

const std::string kSoulStuffFragmentShader = SHADER_TO_STRING(

        precision highp float;
        varying vec2 textureCoordinate;
        uniform sampler2D inputTexture;

        uniform float scale;

        void main() {
            vec2 uv = textureCoordinate.xy;
            // 输入纹理
            vec4 sourceColor = texture2D(inputTexture, fract(uv));
            // 将纹理坐标中心转成(0.0, 0.0)再做缩放
            vec2 center = vec2(0.5, 0.5);
            uv -= center;
            uv = uv / scale;
            uv += center;
            // 缩放纹理
            vec4 scaleColor = texture2D(inputTexture, fract(uv));
            // 线性混合，第三个参数代表alpha值，当alpha为0的时候，显示原图，alpha值越大，scale纹理的分量越大，原图的分量越小
            // mix(x, y, a): x(1-a) + y*a
            // 在混合两种非补色时，会产生一种新的介于他们之间的中间色
            gl_FragColor = mix(sourceColor, scaleColor, 0.5 * (0.6 - fract(scale)));
        }

        );

GLEffectSoulStuffFilter::GLEffectSoulStuffFilter() : scale(1.0f), offset(0.0f), scaleHandle(-1) {

}

void GLEffectSoulStuffFilter::initProgram() {
    initProgram(kDefaultVertexShader.c_str(), kSoulStuffFragmentShader.c_str());
}

void GLEffectSoulStuffFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader, fragmentShader);
    if (isInitialized()) {
        scaleHandle = glGetUniformLocation(programHandle, "scale");
    }
}

void GLEffectSoulStuffFilter::setTimeStamp(double timeStamp) {
    GLFilter::setTimeStamp(timeStamp);
    //LOGD("时间戳：%lf",timeStamp);
    double interval = fmod(timeStamp, 40);
    // offset每次增长的范围是0.0~0.1
    //LOGD("interval的值：%lf",interval);
    offset += interval * 0.0025f;
    // offset的值范围是0.0~1.0
    if (offset >= 1.0f) {
        offset = 0.0f;
    }
    // cos((offset + 1) * PI)/2.0 的值范围是 -0.5~0.5，所以再加0.5后范围就是0.0~1.0，再乘以0.3后范围是0.0~0.3，所以
    // scale的范围是1.0~1.3
    scale = 1.0f + 0.3f * (float)(cos((offset + 1) * PI) / 2.0f + 0.5f);
    //LOGD("scale的值=%f",scale);
}

void GLEffectSoulStuffFilter::onDrawBegin() {
    GLFilter::onDrawBegin();
    if (isInitialized()) {
        glUniform1f(scaleHandle, scale);
    }
}
