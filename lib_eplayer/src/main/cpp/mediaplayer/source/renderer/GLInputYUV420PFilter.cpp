
#include <AndroidLog.h>
#include "GLInputYUV420PFilter.h"
#include "OpenGLUtils.h"

//std::string是标准C++的字符串实现。为了让程序好移植，要用std::string
const std::string kYUV420PFragmentShader = SHADER_TO_STRING(
        precision mediump float;
        varying highp vec2 textureCoordinate;
        uniform lowp sampler2D inputTextureY;
        uniform lowp sampler2D inputTextureU;
        uniform lowp sampler2D inputTextureV;

        void main() {
            vec3 yuv;
            vec3 rgb;
            yuv.r = texture2D(inputTextureY, textureCoordinate).r - (16.0 / 255.0);
            yuv.g = texture2D(inputTextureU, textureCoordinate).r - 0.5;
            yuv.b = texture2D(inputTextureV, textureCoordinate).r - 0.5;
            rgb = mat3(1.164, 1.164, 1.164,
                       0.0, -0.213, 2.112,
                       1.793, -0.533, 0.0) * yuv;
            gl_FragColor = vec4(rgb, 1.0);
        }
);

GLInputYUV420PFilter::GLInputYUV420PFilter() {
    //yuv是3种数据
    for (int i = 0; i < GLES_MAX_PLANE; ++i) {
        inputTextureHandle[i] = 0;
        textures[i] = 0; //初始化纹理句柄
    }
}

GLInputYUV420PFilter::~GLInputYUV420PFilter() {

}

void GLInputYUV420PFilter::initProgram() {
    initProgram(kDefaultVertexShader.c_str(), kYUV420PFragmentShader.c_str());
}

/**
 * 这方法主要作用：创建着色器程序，创建YUV三种对应的纹理对象
 * @param vertexShader
 * @param fragmentShader
 */
void GLInputYUV420PFilter::initProgram(const char *vertexShader, const char *fragmentShader) {
    if (vertexShader && fragmentShader) {
        //创建着色器程序
        programHandle = OpenGLUtils::createProgram(vertexShader, fragmentShader);
        OpenGLUtils::checkGLError("createProgram");

        //glGetAttribLocation查询由program指定的先前链接的程序对象，用于name指定的属性变量，并返回绑定到该属性变量的通用顶点属性的索引
        //获取着色器程序中，指定为attribute类型变量的id
        positionHandle = glGetAttribLocation(programHandle, "aPosition"); //顶点数据的索引
        texCoordinateHandle = glGetAttribLocation(programHandle, "aTextureCoord"); //纹理坐标的索引

        //获取着色器程序中，指定为uniform类型变量的id
        //获取片元着色器里面几个指定的uniform变量的地址，纹理采样器句柄
        inputTextureHandle[0] = glGetUniformLocation(programHandle, "inputTextureY");
        inputTextureHandle[1] = glGetUniformLocation(programHandle, "inputTextureU");
        inputTextureHandle[2] = glGetUniformLocation(programHandle, "inputTextureV");

        //OpenGL默认采用四字节的对齐方式，因此存储一个图像所需的字节数并不完全等于宽高像素字节数
        // GL_UNPACK_ALIGNMENT指定OpenGL如何从数据缓冲区中解包图像数据
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        //加载并使用链接好的程序
        glUseProgram(programHandle);

        //创建纹理对象句柄
        if (textures[0] == 0) { //==0代表还没创建纹理句柄
            //glGenTextures就是用来产生你要操作的纹理对象的索引的，第一个参数是产生的纹理索引数量，第二个是保存纹理索引的
            //创建3个纹理对象句柄，分别是1、2、3
            glGenTextures(3, textures);
            LOGD("纹理句柄%d,%d,%d", textures[0], textures[1], textures[2]);
        }

        //分别对Y U V三个纹理对象进行相关设定，使用3个纹理单元
        for (int i = 0; i < 3; ++i) {
            //激活i号纹理单元并绑定纹理textures[i]，默认情况下0号纹理单元是激活的
            //激活指定纹理单元
            glActiveTexture(GL_TEXTURE0 + i);
            //glBindTexture实际上是改变了OpenGL的这个状态，它告诉OpenGL下面对纹理的任何操作都是对它所绑定的纹理对象的
            //绑定纹理对象句柄到纹理单元，一个纹理单元可以绑定多个纹理对象，只要绑定纹理对象的类型不同，比如1D、2D、3D
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            // 设置纹理过滤模式
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            // 设置纹理坐标包装
            // 将坐标限制在0到1之间。超出的坐标会重复绘制边缘的像素，变成一种扩展边缘的图案
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            //纹理对象和对应的采样器地址进行绑定，该函数用于将纹理对象传入着色器中，第一个参数为采样器句柄，第二个参数为纹理索引
            //其与函数glActiveTexture(GLenum texture)中的参数相对应，即第二个参数等于texture
            //将激活的纹理单元传递到着色器里面，这样片元着色器的纹理句柄就可以通过纹理单元来操作对应的纹理对象了
            glUniform1i(inputTextureHandle[i], i); //函数的全称应该是glUniformLocationint，其实就是给着色器中的Uniform变量赋值的意思
        }
        setInitialized(true);
    } else {
        positionHandle = -1;
        positionHandle = -1;
        inputTextureHandle[0] = -1;
        setInitialized(false);
    }
}

/**
 * 主要是激活和绑定YUV三种纹理对象，并且各自加载YUV三种图片数据
 * 到这里，着色器和纹理对象都已经创建好了，并且纹理对象也已经激活绑定和加载了图片数据，剩下的就是顶点数据和纹理坐标数据的载入，最后绘制纹理
 * @param texture
 * @return
 */
GLboolean GLInputYUV420PFilter::uploadTexture(Texture *texture) {
    // 需要设置1字节对齐
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glUseProgram(programHandle); //使用着色器程序

    // 更新绑定纹理的数据，Y的高度等于图片的高度，U和V的高度等于图片高度的1/2
    const GLsizei heights[3] = {texture->height, texture->height / 2, texture->height / 2};

    for (int i = 0; i < 3; ++i) {
        //激活纹理，这里的纹理对象分别有Y、U、V三种
        glActiveTexture(GL_TEXTURE0 + i);
        //绑定纹理textures[i]，表示后面的任何操作都是对这个纹理textures[i]的
        glBindTexture(GL_TEXTURE_2D, textures[i]);

        /**
        * @param target 目标纹理，即 GL_TEXTURE_2D、GL_TEXTURE_3D、GL_TEXTURE_2D_ARRAY 或 GL_TEXTURE_CUBE_MAP
        * @param level 指定要加载的mip级别。第一个级别为0，后续的mip贴图级别递增
        * @param internalformat 纹理存储的内部格式
        * @param width 图像的像素宽度
        * @param height 图像的像素高度
        * @param border 在OpenGL ES中忽略，保留是为了兼容桌面的OpenGL，传入0
        * @param format 输入的纹理数据格式
        * @param type 输入像素数据的类型
        * @param pixels 包含图像的实际像素数据。像素行必须对齐到用glPixelStroei设置的GL_UNPACK_ALIGNMENT
        */
        //加载图片数据
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_LUMINANCE,
                     texture->pitches[i],
                     heights[i],
                     0,
                     GL_LUMINANCE,
                     GL_UNSIGNED_BYTE,
                     texture->pixels[i]);

        // 纹理对象和对应的采样器地址进行绑定
        // 该函数用于将纹理对象传入着色器中，第二个参数为纹理索引，其与函数glActiveTexture(GL_TEXTURE0 + i)中的参数相对应，如此处应该使用i
        glUniform1i(inputTextureHandle[i], i);
    }
    //像素行必须对齐到用glPixelStroei设置的GL_UNPACK_ALIGNMENT
    glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
    return GL_TRUE;
}

/**
 * 最后一步，渲染纹理
 * @param texture
 * @param vertices 顶点数据
 * @param textureVertices 纹理坐标数据
 * @return
 */
GLboolean
GLInputYUV420PFilter::renderTexture(Texture *texture, float *vertices, float *textureVertices) {
    if (!isInitialized() || !texture) {
        return GL_FALSE;
    }
    // 绑定属性值，顶点坐标和纹理坐标
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
    return GL_TRUE;
}
