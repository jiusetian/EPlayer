//
// Created by Guns Roses on 2020/5/7.
//

#ifndef EPLAYER_GLEFFECTPIPFILTER_H
#define EPLAYER_GLEFFECTPIPFILTER_H

#include <string>
#include <GLFilter.h>
#include "macros.h"

class GLEffectPipFilter : public GLFilter {
public:
    GLEffectPipFilter();

    void initProgram() override;

    void initProgram(const char *vertexShader, const char *fragmentShader) override;

};

// 画中画片元着色器
const std::string PIPFragmentShader = SHADER_TO_STRING(
        precision mediump float;

        // 视频纹理属性
        uniform sampler2D inputTexture; //图片的纹理句柄
        varying vec2 textureCoordinate;


        // 缩放函数
        vec2 scale(vec2 uv, float level)
        {
            vec2 center = vec2(0.5, 0.5);
            vec2 newTexCoord = uv.xy;
            newTexCoord -= center;
            newTexCoord = newTexCoord / level;
            newTexCoord += center;
            return newTexCoord;
        }

        const float OFFSET_LEVEL = 0.15;
        const float SCALE_LEVEL = 1.5;

        void main() {
            if(OFFSET_LEVEL < textureCoordinate.x && textureCoordinate.x < (1.0 - OFFSET_LEVEL)
               && OFFSET_LEVEL < textureCoordinate.y && textureCoordinate.y < (1.0 - OFFSET_LEVEL))
            {
                //将原图下采样到指定区域中
                vec2 newTexCoord = textureCoordinate;
                newTexCoord -= OFFSET_LEVEL;
                newTexCoord = newTexCoord / (1.0 - 2.0 * OFFSET_LEVEL);
                gl_FragColor = texture2D(inputTexture, newTexCoord.xy);
            }
            else
            {
                //原纹理坐标缩放之后再进行采样
                gl_FragColor = texture2D(inputTexture, scale(textureCoordinate, SCALE_LEVEL).xy);//(scale(v_texcoord, SCALE_LEVEL));
            }
        }
);

#endif //EPLAYER_GLEFFECTPIPFILTER_H
