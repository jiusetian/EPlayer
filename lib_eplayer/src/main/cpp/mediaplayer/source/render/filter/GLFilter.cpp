
#include <cstdlib>
#include "GLFilter.h"
#include "render/common/header/OpenGLUtils.h"
#include <AndroidLog.h>

GLFilter::GLFilter() : initialized(false), programHandle(-1), positionHandle(-1), texCoordinateHandle(-1),
                       nb_textures(1), vertexCount(4), timeStamp(0), intensity(1.0), textureWidth(0), textureHeight(0),
                       displayWidth(0), displayHeight(0) {

    for (int i = 0; i < MAX_TEXTURES; ++i) {
        //初始化纹理句柄
        inputTextureHandle[i] = -1;
    }
}

GLFilter::~GLFilter() {
}

void GLFilter::initProgram() {
    if (!isInitialized()) {
        initProgram(kDefaultVertexShader.c_str(), kDefaultFragmentShader.c_str());
    }
}

void GLFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    if (isInitialized()) {
        return;
    }
    if (vertexShader && fragmentShader) {
        programHandle = OpenGLUtils::createProgram(vertexShader, fragmentShader);
        // OpenGLUtils::checkGLError("createProgram");
        // 获取着色器程序中，指定为attribute类型变量的id
        positionHandle = glGetAttribLocation(programHandle, "aPosition");
        texCoordinateHandle = glGetAttribLocation(programHandle, "aTextureCoord");
        // 纹理句柄
        inputTextureHandle[0] = glGetUniformLocation(programHandle, "inputTexture");
        setInitialized(true);
    } else {
        positionHandle = -1;
        positionHandle = -1;
        inputTextureHandle[0] = -1;
        setInitialized(false);
    }
}

void GLFilter::destroyProgram() {
    if (initialized) {
        glDeleteProgram(programHandle);
    }
    programHandle = -1;
}

void GLFilter::setInitialized(bool initialized) {
    this->initialized = initialized;
}

bool GLFilter::isInitialized() {
    return initialized;
}

void GLFilter::setTextureSize(int width, int height) {
    this->textureWidth = width;
    this->textureHeight = height;
}

void GLFilter::setDisplaySize(int width, int height) {
    this->displayWidth = width;
    this->displayHeight = height;
}

void GLFilter::setTimeStamp(double timeStamp) {
    this->timeStamp = timeStamp;
}

void GLFilter::setIntensity(float intensity) {
    this->intensity = intensity;
}

void GLFilter::updateViewPort() {
    if (displayWidth != 0 && displayHeight != 0) {
        LOGD("视口宽高%d，%d",displayWidth,displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);
    } else {
        glViewport(0, 0, textureWidth, textureHeight);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLFilter::drawTexture(GLuint texture, const float *vertices, const float *textureVertices,
                           bool viewPortUpdate) {
    if (!isInitialized() || texture < 0) {
        return;
    }

    if (viewPortUpdate) {
        updateViewPort();
    }

    // 绑定program
    glUseProgram(programHandle);
    // 绑定纹理
    bindTexture(texture);
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
}

void
GLFilter::drawTexture(FrameBuffer *frameBuffer, GLuint texture, const float *vertices, const float *textureVertices) {
    if (frameBuffer) {
        frameBuffer->bindBuffer();
    }
    drawTexture(texture, vertices, textureVertices, false);
    // unbindBuffer相当于返回默认的帧缓冲区
    if (frameBuffer) {
        frameBuffer->unbindBuffer();
    }
}

void GLFilter::bindAttributes(const float *vertices, const float *textureVertices) {
    // 告诉GPU怎么解析顶点数据
    glVertexAttribPointer(positionHandle, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    // 告诉GPU怎么解析纹理坐标数据
    glVertexAttribPointer(texCoordinateHandle, 2, GL_FLOAT, GL_FALSE, 0, textureVertices);

    glEnableVertexAttribArray(positionHandle);
    glEnableVertexAttribArray(texCoordinateHandle);
}

void GLFilter::onDrawBegin() {
}

void GLFilter::onDrawAfter() {
    // do nothing
}

void GLFilter::onDrawFrame() {
    // GL_TRIANGLE_STRIP表示顺序在每三个顶点之间均绘制三角形，这个方法可以保证从相同的方向上所有三角形均被绘制
    // 以V0V1V2,V1V2V3,V2V3V4……的形式绘制三角形
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexCount);
}

void GLFilter::unbindAttributes() {
    glDisableVertexAttribArray(texCoordinateHandle);
    glDisableVertexAttribArray(positionHandle);
}

void GLFilter::bindTexture(GLuint texture) {
    // 默认绑定0号纹理单元
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(getTextureType(), texture);
    glUniform1i(inputTextureHandle[0], 0);
}

void GLFilter::unbindTextures() {
    // 默认解绑纹理单元0
    glBindTexture(getTextureType(), 0);
}

GLenum GLFilter::getTextureType() {
    return GL_TEXTURE_2D;
}
