
#include <AndroidLog.h>
#include "GLInputABGRFilter.h"
#include "render/common/header/OpenGLUtils.h"

const std::string kABGRFragmentShader = SHADER_TO_STRING(
        precision mediump float;
        uniform sampler2D inputTexture;
        varying vec2 textureCoordinate;

        void main() {
            vec4 abgr = texture2D(inputTexture, textureCoordinate);
            // fragColor指定为RGBA格式，原始数据格式是BGRA，应该第一个和第三个交换，即R和B交换
            gl_FragColor = abgr;
            gl_FragColor.r = abgr.b;
            gl_FragColor.b = abgr.r;
        }
);

GLInputABGRFilter::GLInputABGRFilter() {
    for (int i = 0; i < 1; ++i) {
        // 初始化纹理句柄和纹理单元
        inputTextureHandle[i] = 0;
        textures[i] = 0;
    }
}

GLInputABGRFilter::~GLInputABGRFilter() {
}

void GLInputABGRFilter::initProgram() {
    initProgram(inputVertexShader.c_str(), kABGRFragmentShader.c_str());
}

void GLInputABGRFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader, fragmentShader);

    if (isInitialized()) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glUseProgram(programHandle);
        // 设置各纹理对象的参数
        if (textures[0] == 0) {
            glGenTextures(1, textures);
            for (int i = 0; i < 1; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, textures[i]);
                // 纹理参数
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
        }
    }
}

GLboolean GLInputABGRFilter::uploadTexture(Texture *texture) {
    // 需要设置1字节对齐，假如这里像素格式是BGRA8888，即4模式，那就不需要设置1字节对齐，因为默认是4字节对齐的，即一次读取4个字节，所以不管
    // 一行多少个像素，都能准确地读出来，设置1字节对齐的话读取效率会低一点，但是可以保证不会出错
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // 使用着色器程序
    glUseProgram(programHandle);
    // 激活纹理单元0
    glActiveTexture(GL_TEXTURE0);
    // 将纹理对象绑定到纹理单元0
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    // 上载纹理数据
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,                 // 对于YUV来说，数据格式是GL_LUMINANCE亮度值，而对于BGRA来说，这个则是颜色通道值
                 texture->pitches[0] / 4, // 因为这里指定是BGRA8888格式，一个像素4个字节，所以计算像素宽度就要除以4
                 texture->height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 texture->pixels[0]); // 像素数据
    // 将纹理单元0传递给着色器的纹理句柄
    glUniform1i(inputTextureHandle[0], 0);
    return GL_TRUE;
}

void GLInputABGRFilter::onDrawBegin() {
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

