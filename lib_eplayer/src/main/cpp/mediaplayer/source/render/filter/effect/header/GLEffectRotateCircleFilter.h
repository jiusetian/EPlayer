//
// Created by Guns Roses on 2020/5/8.
//

#ifndef EPLAYER_GLEFFECTROTATECIRCLEFILTER_H
#define EPLAYER_GLEFFECTROTATECIRCLEFILTER_H

#include <string>
#include <render/filter/header/GLFilter.h>
#include <render/filter/header/Filter.h>

class GLEffectRotateCircleFilter : public GLFilter {
public:
    GLEffectRotateCircleFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

    // 设置纹理大小
    void setTextureSize(int width, int height) override ;

    // 设置输出大小
    void setDisplaySize(int width, int height) override ;

    void setTimeStamp(double timeStamp) override;


protected:
    void onDrawBegin() override;

private:
    int texSizeHandle;
    int offsetHandle;

    Vector2 texSize;
    float offset;
};

// 画中圆片元着色器
const std::string RotateCircleFragmentShader = SHADER_TO_STRING(
        precision mediump float;
        // 视频纹理属性
        uniform sampler2D inputTexture; //图片的纹理句柄
        varying vec2 textureCoordinate;

        uniform float u_offset ;
        uniform vec2 texSize;

        const float PI = 3.141592653;

        void main() {
            // 纹理坐标转为图片坐标
            vec2 imgTex = textureCoordinate * texSize;
            float r = 0.3 * texSize.x; //设置半径为图片宽度的 0.3 倍

            // 取圆心为中心点半径范围的所有点
            if(distance(imgTex, vec2(texSize.x / 2.0, texSize.y / 2.0)) < r)
            {
                // 移动0.5坐标以后，可以把原点变为中心点，方便计算
                vec2 tranTex = textureCoordinate - 0.5;
                vec2 imgTranTex = tranTex * texSize;
                // 计算当前点的向量长度，即当前点到原点的长度
                float len = length(imgTranTex);
                float angle = 0.0;
                // 计算点向量跟x轴的夹角
                angle = acos(imgTranTex.x / len);
                // 圆的上半部分跟下半部分角度值互为反数
                if(tranTex.y < 0.0)
                {
                    angle *= -1.0;
                }

                // 根据偏移变换角度
                angle -= u_offset;

                // 通过偏移过的角度，改变纹理坐标值，从而改变取得的纹素
                imgTranTex.x = len * cos(angle);
                imgTranTex.y = len * sin(angle);

                vec2 newTexCoors = imgTranTex / texSize + 0.5;

                gl_FragColor = texture2D(inputTexture,newTexCoors.xy);
            }
            else
            {
                gl_FragColor = texture2D(inputTexture,textureCoordinate.xy);
            }
        }
);


#endif //EPLAYER_GLEFFECTROTATECIRCLEFILTER_H
