
#include "render/common/header/OpenGLUtils.h"
#include <AndroidLog.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

GLuint OpenGLUtils::createProgram(const char *vertexShader, const char *fragShader) {
    GLuint vertex;
    GLuint fragment;
    GLuint program;
    GLint linked;

    //加载顶点着色器
    vertex = loadShader(GL_VERTEX_SHADER, vertexShader);
    if (vertex == 0) {
        return 0;
    }
    // 加载片元着色器
    fragment = loadShader(GL_FRAGMENT_SHADER, fragShader);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }

    // 创建program
    program = glCreateProgram();
    if (program == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }
    // 绑定shader
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    // 链接program程序
    glLinkProgram(program);
    // 检查链接状态
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        // 检查日志信息长度
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            // 分配一个足以存储日志信息的字符串
            char *infoLog = (char *) malloc(sizeof(char) * infoLen);
            // 检索日志信息
            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            LOGE("Error linking program:\n%s\n", infoLog);
            // 使用完成后需要释放字符串分配的内存
            free(infoLog);
        }
        // 删除着色器释放内存
        glDetachShader(program, vertex);
        glDeleteShader(vertex);
        glDetachShader(program, fragment);
        glDeleteShader(fragment);
        glDeleteProgram(program);
        return 0;
    }
    // 删除着色器释放内存
    glDetachShader(program, vertex);
    glDeleteShader(vertex);
    glDetachShader(program, fragment);
    glDeleteShader(fragment);

    return program;
}

GLuint OpenGLUtils::loadShader(GLenum type, const char *shaderSrc) {

    GLuint shader;
    GLint compiled;
    // 创建shader
    shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }
    // 加载着色器的源码
    glShaderSource(shader, 1, &shaderSrc, NULL);

    // 编译源码
    glCompileShader(shader);

    // 检查编译状态
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) { //编译出错
        GLint infoLen = 0;
        // 查询日志的长度判断是否有日志产生
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1) {
            // 分配一个足以存储日志信息的字符串
            char *infoLog = (char *) malloc(sizeof(char) * infoLen);
            // 检索日志信息
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            LOGE("Error compiling shader:\n%s\n", infoLog);
            // 使用完成后需要释放字符串分配的内存
            free(infoLog);
        }
        // 删除编译出错的着色器释放内存
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void OpenGLUtils::checkActiveUniform(GLuint program) {
    GLint maxLen;
    GLint numUniforms;
    char *uniformName;

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLen);

    uniformName = (char *) malloc(sizeof(char) * maxLen);

    for (int i = 0; i < numUniforms; ++i) {
        GLint size;
        GLenum type;
        GLint location;

        glGetActiveUniform(program, i, maxLen, NULL, &size, &type, uniformName);

        location = glGetUniformLocation(program, uniformName);

        LOGD("location:", location);

        switch (type) {
            case GL_FLOAT: {
                LOGD("type : GL_FLOAT");
                break;
            }
            case GL_FLOAT_VEC2: {
                LOGD("type : GL_FLOAT_VEC2");
                break;
            }
            case GL_FLOAT_VEC3: {
                LOGD("type : GL_FLOAT_VEC3");
                break;
            }
            case GL_FLOAT_VEC4: {
                LOGD("type : GL_FLOAT_VEC4");
                break;
            }
            case GL_INT: {
                LOGD("type : GL_INT");
                break;
            }
        }
    }
}

GLuint OpenGLUtils::createTexture(GLenum type) {
    GLuint textureId;
    // 设置解包对齐
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // 创建纹理
    glGenTextures(1, &textureId);
    // 绑定纹理
    glBindTexture(type, textureId);
    // 设置放大缩小模式
    glTexParameterf(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return textureId;
}

GLuint OpenGLUtils::createTextureWithBytes(unsigned char *bytes, int width, int height) {
    GLuint textureId;
    if (bytes == NULL) {
        return 0;
    }
    // 创建Texture
    glGenTextures(1, &textureId);
    // 绑定类型
    glBindTexture(GL_TEXTURE_2D, textureId);
    // 利用像素创建texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
    // 设置放大缩小模式
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return textureId;
}

GLuint OpenGLUtils::createTextureWithOldTexture(GLuint texture, unsigned char *bytes, int width,
                                                int height) {
    if (texture == 0) {
        return createTextureWithBytes(bytes, width, height);
    }
    // 绑定到当前的Texture
    glBindTexture(GL_TEXTURE_2D, texture);
    // 更新Texture数据
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_NONE, GL_TEXTURE_2D, bytes);
    return texture;
}

void OpenGLUtils::createFrameBuffer(GLuint *framebuffer, GLuint *texture, int width, int height) {
    createFrameBuffers(framebuffer, texture, width, height, 1);
}

void OpenGLUtils::createFrameBuffers(GLuint *frambuffers, GLuint *textures, int width, int height,
                                     int size) {
    // 创建FBO
    glGenFramebuffers(size, frambuffers);
    // 创建Texture
    glGenTextures(size, textures);
    for (int i = 0; i < size; ++i) {
        // 绑定Texture
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        // 创建一个没有像素的的Texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        // 设置放大缩小模式
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // 创建完成后需要解绑
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void OpenGLUtils::checkGLError(const char *op) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        LOGE("[GLES2] after %s() glError (0x%x)\n", op, error);
    }
}

void OpenGLUtils::bindTexture(int location, int texture, int index) {
    bindTexture(location, texture, index, GL_TEXTURE_2D);
}

void OpenGLUtils::bindTexture(int location, int texture, int index, int textureType) {
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(textureType, texture);
    glUniform1i(location, index);
}

glm::mat4 OpenGLUtils::caculateVideoFitMat4(int videoWidth, int videoHeight, int displayWidth, int displayHeight) {
    glm::mat4 v_mat4;//创建单位矩阵
    float v_ratio = (float) videoWidth / videoHeight;
    float s_ratio = (float) displayWidth / displayHeight;
    //1280, 720, 1536, 2048
    float left = -1.0f;
    float right = 1.0f;
    float top = 1.0f;
    float bottom = -1.0f;

    if ((displayWidth > displayHeight && v_ratio > s_ratio) || (displayWidth < displayHeight && v_ratio > s_ratio)) {
        //缩放高度
        top = v_ratio / s_ratio;
        bottom = -(v_ratio / s_ratio);
    } else if ((displayWidth > displayHeight && v_ratio < s_ratio) ||
               (displayWidth < displayHeight && v_ratio < s_ratio)) {

        //缩放宽度
        right = s_ratio / v_ratio;
        left = -right;
    }
    //矩阵运算
    //v_mat4=glm::ortho(-1.0f,1.0f, -1.0f, 1.0f);
    //v_mat4 = glm::ortho(left, right, -2.37037037f, 2.37037037f);
    LOGD("左右上下%f,%f,%f,%f",left,right,top,bottom);
    v_mat4 = glm::ortho(left, right, bottom, top);
    return v_mat4;
}

std::string *OpenGLUtils::readShaderFromAsset(const char *fileName) {
    //1.读取着色器的代码
    string fragmentCode;
    ifstream fShaderFile;
    //确保文件流会输出异常
    fShaderFile.exceptions(ifstream::failbit | ifstream::badbit);
    try {
        //打开文件
        fShaderFile.open(fileName);
        stringstream fShaderStream;
        //读取文件到流中
        //在 C++ 编程中，我们使用流提取运算符（ >> ）从文件读取信息，就像使用该运算符从键盘输入信息一样
        //唯一不同的是，在这里您使用的是 ifstream 或 fstream 对象，而不是 cin 对象
        fShaderStream << fShaderFile.rdbuf();
        //关闭文件
        fShaderFile.close();
        //将流转换为字符串
        fragmentCode = fShaderStream.str();

        return &fragmentCode;
    } catch (ifstream::failure) {
        LOGD("读取文件失败，请检查文件是否存在");
    }
}
