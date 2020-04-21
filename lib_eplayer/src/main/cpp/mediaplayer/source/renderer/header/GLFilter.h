
#ifndef EPLAYER_GLFILTER_H
#define EPLAYER_GLFILTER_H

#if defined(__ANDROID__)

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#endif

#include "glm.hpp"
#include <gtc/type_ptr.inl>
#include <gtx/rotate_vector.hpp>
#include <string>
#include "FrameBuffer.h"
#include "macros.h"
#include "Shader.h"


using namespace std;

// OpenGLES 2.0 最大支持32个纹理
#define MAX_TEXTURES 32

/**
 * 默认的vertex shader顶点着色器
 */
const std::string kDefaultVertexShader = SHADER_TO_STRING(
        precision mediump float; //默认精度
        //顶点坐标，x、y、z、w四个值
        attribute highp vec4 aPosition;
        //纹理坐标
        attribute highp vec2 aTextureCoord;
        //纹理坐标传递到片元着色器的变量，通过varying通道传递
        varying vec2 textureCoordinate;
        //正交矩阵
        uniform mat4 uMatrix;

        void main() {
            gl_Position = uMatrix * vec4(aPosition.x, aPosition.y, aPosition.z, aPosition.w);
            textureCoordinate = aTextureCoord.xy;
        }
);

/**
 * 默认的 fragment shader
 */
const std::string kDefaultFragmentShader = SHADER_TO_STRING(
        precision mediump float;
        uniform sampler2D inputTexture;
        varying vec2 textureCoordinate;

        void main() {
            gl_FragColor = texture2D(inputTexture, textureCoordinate.xy);
        }
);


/**
 * 滤镜基类，简单的说滤镜就是图像的效果处理，它基本功能是对图像形变(利用顶点着色器)和色彩(利用片段着色器)的操控
 * 滤镜也是绘图软件中用于制造特殊效果的工具统称，以Photoshop为例，它拥有风格化、画笔描边、模糊、扭曲、锐化、视频
 * 素描、纹理、像素化、渲染、艺术效果、其他等12个滤镜
 */
class GLFilter {
public:
    GLFilter();

    virtual ~GLFilter();

    // 初始化program
    virtual void initProgram();

    // 初始化program
    virtual void initProgram(const char *vertexShader, const char *fragmentShader);

    // 销毁program
    virtual void destroyProgram();

    // 更新viewport值
    void updateViewPort();

    // 直接绘制纹理
    virtual void
    drawTexture(GLuint texture, const float *vertices, const float *textureVertices, bool viewPortUpdate = true);

    // 将纹理绘制到FBO中，实际上就是RenderNode中创建的FrameBuffer。这个FBO可以不跟随GLFilter释放，单独维护
    virtual void
    drawTexture(FrameBuffer *frameBuffer, GLuint texture, const float *vertices, const float *textureVertices);

    // 设置纹理大小
    virtual void setTextureSize(int width, int height);

    // 设置输出大小
    virtual void setDisplaySize(int width, int height);

    // 设置时间戳
    virtual void setTimeStamp(double timeStamp);

    // 设置强度
    virtual void setIntensity(float intensity);

    // 设置是否初始化
    virtual void setInitialized(bool initialized);

    // 是否已经初始化
    virtual bool isInitialized();

    //Surface的大小发生改变
    void nativeSurfaceChanged(int width, int height);


protected:
    // 绑定attribute属性
    virtual void bindAttributes(const float *vertices, const float *textureVertices);

    //绑定uniform属性
    virtual void bindUniforms();

    // 绑定纹理
    virtual void bindTexture(GLuint texture);

    // 绘制之前处理
    virtual void onDrawBegin();

    // 绘制之后处理
    virtual void onDrawAfter();

    // 绘制方法
    virtual void onDrawFrame();

    // 解绑attribute属性
    virtual void unbindAttributes();

    // 解绑纹理
    virtual void unbindTextures();

    // 绑定的纹理类型，默认为GL_TEXTURE_2D
    virtual GLenum getTextureType();

protected:
    bool initialized;       // shader program 初始化标志
    int programHandle;      // 程序句柄
    int positionHandle;     // 顶点坐标句柄
    int texCoordinateHandle;// 纹理坐标句柄
    int inputTextureHandle[MAX_TEXTURES]; // 纹理句柄列表
    int matrixHandle;       // 顶点坐标的矩阵句柄
    int nb_textures;        // 纹理数量
    int vertexCount = 4;    // 绘制的顶点个数，默认为4
    double timeStamp;       // 时间戳
    float intensity;        // 强度 0.0 ~ 1.0，默认为1.0
    int textureWidth;       // 纹理宽度
    int textureHeight;      // 纹理高度
    int displayWidth;       // 显示输出宽度
    int displayHeight;      // 显示输出高度
    glm::mat4 v_mat4 = glm::mat4(1.0f);       //顶点的矩阵
    int surfaceWidth;       // surface的宽度
    int surfaceHeight;      // Surface的高度
};

#endif //EPLAYER_GLFILTER_H
