
#include <AndroidLog.h>
#include "GLInputABGRFilter.h"
#include "OpenGLUtils.h"

const std::string kABGRFragmentShader = SHADER_TO_STRING(
        precision mediump float;
        uniform sampler2D inputTexture;
        varying vec2 textureCoordinate;

        void main() {
            vec4 abgr = texture2D(inputTexture, textureCoordinate);
            //因为fragColor的格式是argb，而这里是abgr，所以b和r需要交换一下
            gl_FragColor = abgr;
            gl_FragColor.r = abgr.b;
            gl_FragColor.b = abgr.r;
        }
);

const std::string kABGRFragmentShader1 = SHADER_TO_STRING(
        precision mediump float;
        uniform int iFilterType;
        uniform vec3 vFilterColor;
        
        uniform sampler2D inputTexture;
        varying vec2 textureCoordinate;

        void main() {
            //转换一下格式，texture2D是2.0的函数，3.0要用texture
            vec4 abgr = texture2D(inputTexture, textureCoordinate);
            vec4 tmpColor=abgr;
            tmpColor.r = abgr.b;
            tmpColor.b = abgr.r;
            gl_FragColor=tmpColor;

            if (iFilterType == 1) { // 灰度图
                float c = tmpColor.r * vFilterColor.r + tmpColor.g * vFilterColor.g + tmpColor.b * vFilterColor.b;
                gl_FragColor = vec4(c, c, c, tmpColor.a);
            } else if (iFilterType == 2) { // 冷暖色
                tmpColor.r = min(1.0, tmpColor.r + vFilterColor.r);
                tmpColor.g = min(1.0, tmpColor.g + vFilterColor.g);
                tmpColor.b = min(1.0, tmpColor.b + vFilterColor.b);
                gl_FragColor = tmpColor;
            } else if (iFilterType == 3) { // 模糊
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x - vFilterColor.r,textureCoordinate.y - vFilterColor.r));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x - vFilterColor.r,textureCoordinate.y + vFilterColor.r));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x + vFilterColor.r,textureCoordinate.y - vFilterColor.r));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x + vFilterColor.r,textureCoordinate.y + vFilterColor.r));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x - vFilterColor.g,textureCoordinate.y - vFilterColor.g));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x - vFilterColor.g,textureCoordinate.y + vFilterColor.g));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x + vFilterColor.g,textureCoordinate.y - vFilterColor.g));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x + vFilterColor.g,textureCoordinate.y + vFilterColor.g));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x - vFilterColor.b,textureCoordinate.y - vFilterColor.b));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x - vFilterColor.b,textureCoordinate.y + vFilterColor.b));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x + vFilterColor.b,textureCoordinate.y - vFilterColor.b));
                tmpColor += texture2D(inputTexture, vec2(textureCoordinate.x + vFilterColor.b,textureCoordinate.y + vFilterColor.b));
                tmpColor /= 13.0;
                gl_FragColor = tmpColor;
            } else {
                gl_FragColor = tmpColor;
            }
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
    initProgram(kDefaultVertexShader.c_str(), kABGRFragmentShader1.c_str());
}

void GLInputABGRFilter::bindUniforms() {
    GLFilter::bindUniforms();
    glUniform1i(mFilterTypeLoc, filterState->getFilterType());
    glUniform3fv(mFilterColorLoc, 1, filterState->getFilterColor());
}

void GLInputABGRFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    GLFilter::initProgram(vertexShader, fragmentShader);

    if (isInitialized()) {
        mFilterTypeLoc = glGetUniformLocation(programHandle, "iFilterType");
        mFilterColorLoc = glGetUniformLocation(programHandle, "vFilterColor");
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glUseProgram(programHandle);
        //设置各纹理对象的参数
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
    //使用着色器程序
    glUseProgram(programHandle);
    //激活纹理单元0
    glActiveTexture(GL_TEXTURE0);
    //将纹理对象绑定到纹理单元
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    //上载纹理数据
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,                 // 对于YUV来说，数据格式是GL_LUMINANCE亮度值，而对于BGRA来说，这个则是颜色通道值
                 texture->pitches[0] / 4, // pixels中存放的数据是BGRABGRABGRA方式排列的，这里除4是为了求出对齐后的宽度，也就是每个颜色通道的数值
                 texture->height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 texture->pixels[0]);
    //将纹理单元传递给着色器的inputTextureHandle[0]句柄
    glUniform1i(inputTextureHandle[0], 0);
    return GL_TRUE;
}

GLboolean GLInputABGRFilter::renderTexture(Texture *texture, float *vertices, float *textureVertices) {
    if (!texture || !isInitialized()) {
        return GL_FALSE;
    }
    // 绑定属性值
    bindAttributes(vertices, textureVertices);
    // 绑定uniform
    bindUniforms();
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

