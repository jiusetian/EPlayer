
#include <AndroidLog.h>
#include "renderer/header/GLInputABGRFilter.h"

const std::string kABGRFragmentShader = SHADER_TO_STRING(
        precision
        mediump float;
        uniform
        sampler2D inputTexture;
        varying
        vec2 textureCoordinate;

        void main() {
            vec4 abgr = texture2D(inputTexture, textureCoordinate);
            gl_FragColor = abgr;
            gl_FragColor.r = abgr.b;
            gl_FragColor.b = abgr.r;
        }
);

const std::string FragmentShader=SHADER_TO_STRING(
        precision lowp float;
        precision lowp int;
        uniform sampler2D vTexture;
        uniform int iternum;
        uniform float aaCoef; //参数
        uniform float mixCoef; //混合系数
        varying highp vec2 textureCoordinate;
        varying highp vec2 blurCoord1s[14];
        const float distanceNormalizationFactor = 4.0;    //标准化距离因子常量
        const mat3 saturateMatrix = mat3(1.1102,-0.0598,-0.061,-0.0774,1.0826,-0.1186,-0.0228,-0.0228,1.1772);

        void main( ) {

            vec3 centralColor;
            float central;
            float gaussianWeightTotal;
            float sum;
            float sampleColor;
            float distanceFromCentralColor;
            float gaussianWeight;

            //通过绿色通道来磨皮
            //取得当前点颜色的绿色通道
            central = texture2D( vTexture, textureCoordinate ).g;
            //高斯权重
            gaussianWeightTotal = 0.2;
            //绿色通道色彩记数
            sum = central * 0.2;

            // 计算各个采样点处的高斯权重，包括密闭性和相似性
            for (int i = 0; i < 6; i++) {
                //采样点的绿色通道
                sampleColor = texture2D( vTexture, blurCoord1s[i] ).g;
                //采样点和计算点的颜色差
                distanceFromCentralColor = min( abs( central - sampleColor ) * distanceNormalizationFactor, 1.0 );
                //高斯权重
                gaussianWeight = 0.05 * (1.0 - distanceFromCentralColor);
                //高斯权重总和
                gaussianWeightTotal += gaussianWeight;
                //绿色通道色彩记数累计
                sum += sampleColor * gaussianWeight;
            }
            for (int i = 6; i < 14; i++) {
                //采样点的绿色通道
                sampleColor = texture2D( vTexture, blurCoord1s[i] ).g;
                //采样点和计算点的颜色差
                distanceFromCentralColor = min( abs( central - sampleColor ) * distanceNormalizationFactor, 1.0 );
                //高斯权重
                gaussianWeight = 0.1 * (1.0 - distanceFromCentralColor);
                //高斯权重总和
                gaussianWeightTotal += gaussianWeight;
                //绿色通道色彩记数累计
                sum += sampleColor * gaussianWeight;
            }

            //采样后的绿色通道色彩均值
            sum = sum / gaussianWeightTotal;

            //取得当前点的颜色
            centralColor = texture2D( vTexture, textureCoordinate ).rgb;
            //采样值
            sampleColor = centralColor.g - sum + 0.5;
            //迭代计算
            for (int i = 0; i < iternum; ++i) {
                if (sampleColor <= 0.5) {
                    sampleColor = sampleColor * sampleColor * 2.0;
                }else {
                    sampleColor = 1.0 - ((1.0 - sampleColor)*(1.0 - sampleColor) * 2.0);
                }
            }

            float aa = 1.0 + pow( centralColor.g, 0.3 )*aaCoef;
            vec3 smoothColor = centralColor*aa - vec3( sampleColor )*(aa - 1.0);
            smoothColor = clamp( smoothColor, vec3( 0.0 ), vec3( 1.0 ) );
            smoothColor = mix( centralColor, smoothColor, pow( centralColor.g, 0.33 ) );
            smoothColor = mix( centralColor, smoothColor, pow( centralColor.g, mixCoef ) );
            gl_FragColor = vec4( pow( smoothColor, vec3( 0.96 ) ), 1.0 );
            vec3 satcolor = gl_FragColor.rgb * saturateMatrix;
            gl_FragColor.rgb = mix( gl_FragColor.rgb, satcolor, 0.23 );

        }
        );

const std::string VertexShader=SHADER_TO_STRING(
        attribute vec4 vPosition;
        attribute vec2 vCoord;
        varying vec2 textureCoordinate;
        varying vec2 blurCoord1s[14];
        const highp float mWidth=720.0;
        const highp float mHeight=1280.0;
        uniform mat4 vMatrix;
        void main( )
        {
            gl_Position = vMatrix*vPosition;
            textureCoordinate = vCoord;

            highp float mul_x = 2.0 / mWidth;
            highp float mul_y = 2.0 / mHeight;

            // 14个采样点
            blurCoord1s[0] = vCoord + vec2( 0.0 * mul_x, -10.0 * mul_y );
            blurCoord1s[1] = vCoord + vec2( 8.0 * mul_x, -5.0 * mul_y );
            blurCoord1s[2] = vCoord + vec2( 8.0 * mul_x, 5.0 * mul_y );
            blurCoord1s[3] = vCoord + vec2( 0.0 * mul_x, 10.0 * mul_y );
            blurCoord1s[4] = vCoord + vec2( -8.0 * mul_x, 5.0 * mul_y );
            blurCoord1s[5] = vCoord + vec2( -8.0 * mul_x, -5.0 * mul_y );
            blurCoord1s[6] = vCoord + vec2( 0.0 * mul_x, -6.0 * mul_y );
            blurCoord1s[7] = vCoord + vec2( -4.0 * mul_x, -4.0 * mul_y );
            blurCoord1s[8] = vCoord + vec2( -6.0 * mul_x, 0.0 * mul_y );
            blurCoord1s[9] = vCoord + vec2( -4.0 * mul_x, 4.0 * mul_y );
            blurCoord1s[10] = vCoord + vec2( 0.0 * mul_x, 6.0 * mul_y );
            blurCoord1s[11] = vCoord + vec2( 4.0 * mul_x, 4.0 * mul_y );
            blurCoord1s[12] = vCoord + vec2( 6.0 * mul_x, 0.0 * mul_y );
            blurCoord1s[13] = vCoord + vec2( 4.0 * mul_x, -4.0 * mul_y );
        }
        );


GLInputABGRFilter::GLInputABGRFilter() {
    for (int i = 0; i < 1; ++i) {
        inputTextureHandle[i] = 0;
        textures[i] = 0;
    }
}

GLInputABGRFilter::~GLInputABGRFilter() {

}

void GLInputABGRFilter::initProgram() {
    initProgram(kDefaultVertexShader.c_str(), kABGRFragmentShader.c_str());
}

void GLInputABGRFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader, fragmentShader);
    if (isInitialized()) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glUseProgram(programHandle);

        if (textures[0] == 0) {
            glGenTextures(1, textures);
            for (int i = 0; i < 1; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, textures[i]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
        }
    }
}

GLboolean GLInputABGRFilter::uploadTexture(Texture *texture) {
    // 需要设置1字节对齐
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glUseProgram(programHandle);
    // 更新纹理数据
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,                 // 对于YUV来说，数据格式是GL_LUMINANCE亮度值，而对于BGRA来说，这个则是颜色通道值
                 texture->pitches[0] / 4, // pixels中存放的数据是BGRABGRABGRA方式排列的，这里除4是为了求出对齐后的宽度，也就是每个颜色通道的数值
                 texture->height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 texture->pixels[0]);
    glUniform1i(inputTextureHandle[0], 0);
    return GL_TRUE;
}

GLboolean GLInputABGRFilter::renderTexture(Texture *texture, float *vertices, float *textureVertices) {
    if (!texture || !isInitialized()) {
        return GL_FALSE;
    }
    // 绑定属性值
    bindAttributes(vertices, textureVertices);
    // 绘制前处理
    onDrawBegin();
    // 绘制纹理
    onDrawFrame();
    // 绘制后处理
    onDrawAfter();
    // 解绑属性
    unbindAttributes();
    // 解绑纹理
    unbindTextures();
    // 解绑program
    glUseProgram(0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
    return GL_TRUE;
}

