
#include "CoordinateUtils.h"
#include "AndroidLog.h"

static const float vertices_default[] = {
        -1.0f, -1.0f,  // left,  bottom,代表物体的左下角对应OpenGL的坐标值，这个值也是OpenGL坐标系的左下角
        1.0f, -1.0f,  // right, bottom
        -1.0f, 1.0f,  // left,  top
        1.0f, 1.0f,  // right, top

//        -1.0f, -0.5f,  // left,  bottom,代表物体的左下角对应OpenGL的坐标值，这个值也是OpenGL坐标系的左下角
//        1.0f, -0.5f,  // right, bottom
//        -1.0f, 0.5f,  // left,  top
//        1.0f, 0.5f,  // right, top
};

static const float texture_vertices_none_input[] = {
        //按道理来说，纹理坐标的原点应该是左下角，但是由于Android屏幕的原点是左上角，所以这里的纹理坐标也按左上角为原点计算
        //纹理坐标跟顶点坐标应该是一一对应的，比如顶点坐标集合的第一个坐标是左下角，那么纹理坐标集合的第一个坐标也应该是左下角
        0.0f, 1.0f, // 左下，代表着纹理坐标系中这个坐标点位置的像素渲染到上面顶点坐标组的左下角坐标
        1.0f, 1.0f, // 右下
        0.0f, 0.0f, // 左上
        1.0f, 0.0f, // 右上

        //下面的坐标会导致画面倒立
//        0.0f, 0.0f, // left,  bottom
//        1.0f, 0.0f, // right, bottom
//        0.0f, 1.0f, // left,  top
//        1.0f, 1.0f, // right, top
};


//顶点数组的索引数组，这个是引导OpenGL怎么通过画两个三角形来形成矩形的
static const short indices_default[] = {
        0, 1, 2,
        2, 1, 3,
};

// 这个纹理坐标是以左下角为原点的，用于帧缓冲区的纹理坐标
static const float texture_vertices_none[] = {
        0.0f, 0.0f, // left,  bottom
        1.0f, 0.0f, // right, bottom
        0.0f, 1.0f, // left,  top
        1.0f, 1.0f, // right, top
};

/**
 * 先上面，后下面
 */
static const float vertices_default1[] = {
        -1.0f, 1.0f,  // left,  top
        1.0f, 1.0f,  // right, top
        -1.0f, -1.0f,  // left,  bottom,代表物体的左下角对应OpenGL的坐标值，这个值也是OpenGL坐标系的左下角
        1.0f, -1.0f,  // right, bottom
};
/**
 * 先上面，后下面
 */
static const float texture_vertices_none_input1[] = {
        0.0f, 0.0f, // 左上
        1.0f, 0.0f, // 右上
        0.0f, 1.0f, // 左下，代表着纹理坐标系中这个坐标点位置的像素渲染到上面顶点坐标组的左下角坐标
        1.0f, 1.0f, // 右下
};

static const float texture_vertices_90[] = {
        1.0f, 0.0f, // right, bottom
        1.0f, 1.0f, // right, top
        0.0f, 0.0f, // left,  bottom
        0.0f, 1.0f, // left,  top
};

static const float texture_vertices_90_input[] = {
        1.0f, 1.0f, // right, top
        1.0f, 0.0f, // right, bottom
        0.0f, 1.0f, // left,  top
        0.0f, 0.0f, // left,  bottom
};

static const float texture_vertices_180[] = {
        1.0f, 1.0f, // righ,  top
        0.0f, 1.0f, // left,  top
        1.0f, 0.0f, // right, bottom
        0.0f, 0.0f, // left,  bottom
};

static const float texture_vertices_180_input[] = {
        1.0f, 0.0f, // right, bottom
        0.0f, 0.0f, // left,  bottom
        1.0f, 1.0f, // right, top
        0.0f, 1.0f, // left,  top
};

static const float texture_vertices_270[] = {
        0.0f, 1.0f, // left,  top
        0.0f, 0.0f, // left,  bottom
        1.0f, 1.0f, // right, top
        1.0f, 0.0f, // right, bottom
};

static const float texture_vertices_270_input[] = {
        0.0f, 0.0f, // left,  bottom
        0.0f, 1.0f, // left,  top
        1.0f, 0.0f, // right, bottom
        1.0f, 1.0f, // right, top
};

static const float texture_vertices_flip_vertical[] = {
        0.0f, 1.0f, // left,  top
        1.0f, 1.0f, // right, top
        0.0f, 0.0f, // left,  bottom
        1.0f, 0.0f, // right, bottom
};

static const float texture_vertices_flip_horizontal[] = {
        1.0f, 0.0f, // right, bottom
        0.0f, 0.0f, // left,  bottom
        1.0f, 1.0f, // right, top
        0.0f, 1.0f, // left,  top
};

const float *CoordinateUtils::getVertexCoordinates() {
    return vertices_default;
}

const short *CoordinateUtils::getDefaultIndices() {
    return indices_default;
}

const float *CoordinateUtils::getTextureCoordinates(const RotationMode &rotationMode) {
    switch (rotationMode) {
        case ROTATE_NONE: {
            return texture_vertices_none;
        }
        case ROTATE_90: {
            return texture_vertices_90;
        }

        case ROTATE_180: {
            return texture_vertices_180;
        }

        case ROTATE_270: {
            return texture_vertices_270;
        }

        case ROTATE_FLIP_VERTICAL: {
            return texture_vertices_flip_vertical;
        }

        case ROTATE_FLIP_HORIZONTAL: {
            return texture_vertices_flip_horizontal;
        }
    }
    return texture_vertices_none;
}


const float *CoordinateUtils::getInputTextureCoordinates(const RotationMode &rotationMode) {
    switch (rotationMode) {

        case ROTATE_NONE: {
            return texture_vertices_none_input;
        }
        case ROTATE_90: {
            return texture_vertices_90_input;
        }

        case ROTATE_180: {
            return texture_vertices_180_input;
        }

        case ROTATE_270: {
            return texture_vertices_270_input;
        }

        case ROTATE_FLIP_VERTICAL: {
            return texture_vertices_flip_vertical;
        }

        case ROTATE_FLIP_HORIZONTAL: {
            return texture_vertices_flip_horizontal;
        }
    }
    return texture_vertices_none_input;
}
