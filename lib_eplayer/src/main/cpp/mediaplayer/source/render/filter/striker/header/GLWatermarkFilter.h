//
// Created by Guns Roses on 2020/5/4.
//

#ifndef EPLAYER_GLWATERMARKFILTER_H
#define EPLAYER_GLWATERMARKFILTER_H

#include <string>
#include <GLFilter.h>
#include "macros.h"
#include "glm.hpp"
#include <gtc/matrix_transform.hpp>
#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>

class GLWatermarkFilter : public GLFilter {

public:

    GLWatermarkFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    void setWatermark(uint8_t *watermarkPixel, size_t length, GLint width, GLint height,GLfloat scale,GLint location);

    // 绑定attribute属性
    void bindAttributes(const float *vertices, const float *textureVertices) override;

    // 绑定纹理
    void bindTexture(GLuint texture) override;

    // 解绑attribute属性
    void unbindAttributes() override;

protected:
    void onDrawBegin() override;

private:
    int mWatermarkTexCoord;
    // 水印纹理对象
    GLuint mWatermarkTexture;

    GLint mWatermarkWidth; //水印宽
    GLint mWatermarkHeight; //水印高
    GLint mMatrixHandle; //水印纹理矩阵
    uint8_t *mWatermarkPixel; //水印纹理数据，一个byte指针

    glm::mat4 v_mat4 = glm::mat4(1.0f); // 矩阵
};

// 水印顶点着色器
const std::string WatermarkVertexShader = SHADER_TO_STRING(
        precision mediump float; //默认精度

        // 顶点坐标，x、y、z、w四个值
        attribute highp vec4 aPosition;
        // 视频纹理坐标
        attribute highp vec4 aTextureCoord;
        // 通过varying通道传递给片元着色器
        varying vec2 textureCoordinate;

        // 水印纹理坐标
        attribute highp vec4 watermarkTextureCoord;
        // 水印纹理坐标varying传递通道
        varying vec2 watermarkTexCoordinate;
        // 水印纹理矩阵
        uniform mat4 mWatermarkMatrix;

        void main() {
            textureCoordinate = aTextureCoord.xy;
            // 这里改变了水印的纹理坐标，所以纹理图像要变小，矩阵的数据要大于1，因为本来当前顶点到变换前的纹理坐标取纹素的，纹理坐标加倍变大
            // 以后，就变成原来对应位置的纹素到加倍后的纹理坐标那里采取，所以图像整体上就变小了，然后图像纹理坐标超出1的范围就没有颜色了，所以
            // 就是透明的了，就可以设置显示为视频的画面
            watermarkTexCoordinate = (mWatermarkMatrix * watermarkTextureCoord).xy; //通过矩阵改变纹理坐标
            gl_Position = vec4(aPosition.x, aPosition.y, aPosition.z, aPosition.w);
        }
);

// 水印片元着色器
const std::string WatermarkFragmentShader = SHADER_TO_STRING(
        precision mediump float;

        // 视频纹理属性
        uniform sampler2D inputTexture; //图片的纹理句柄
        varying vec2 textureCoordinate;

        // 水印纹理属性
        uniform sampler2D watermarkTexture; //水印纹理句柄
        varying vec2 watermarkTexCoordinate; //水印纹理坐标

        void main() {
            vec4 video = texture2D(inputTexture, textureCoordinate.xy);
            vec4 watermark = texture2D(watermarkTexture, watermarkTexCoordinate.xy);
            // 水印之外的区域显示视频画面，
            float r = watermark.r + (1.0 - watermark.a) * video.r;
            float g = watermark.g + (1.0 - watermark.a) * video.g;
            float b = watermark.b + (1.0 - watermark.a) * video.b;
            gl_FragColor = vec4(r, g, b, 1.0);
            //gl_FragColor =watermark;
        }
);

#endif //EPLAYER_GLWATERMARKFILTER_H
