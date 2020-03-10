
#include "CoordinateUtils.h"
#include "AndroidLog.h"

static const float vertices_default[] = {
        -1.0f, -1.0f,  // left,  bottom
        1.0f, -1.0f,  // right, bottom
        -1.0f, 1.0f,  // left,  top
        1.0f, 1.0f,  // right, top

//        -0.5f, -0.5f,  // left,  bottom
//        0.5f, -0.5f,  // right, bottom
//        -0.5f, 0.5f,  // left,  top
//        0.5f, 0.5f,  // right, top
};

static const short indices_default[] = {
        0, 1, 2,
        2, 1, 3,
};

static const float texture_vertices_none[] = {
        0.0f, 0.0f, // left,  bottom
        1.0f, 0.0f, // right, bottom
        0.0f, 1.0f, // left,  top
        1.0f, 1.0f, // right, top
};

static const float texture_vertices_none_input[] = {
        //按道理来说，在纹理贴图的时候，这个纹理的4个坐标点跟OpenGL的4个坐标点是一一对应的，但事实并不是，可能跟Android的坐标系原点在左上角有关
        0.0f, 1.0f, // left, top
        1.0f, 1.0f, // right, top
        0.0f, 0.0f, // left, bottom
        1.0f, 0.0f, // right, bottom

        //用下面的坐标会导致画面倒立
//        0.0f, 0.0f, // left,  bottom
//        1.0f, 0.0f, // right, bottom
//        0.0f, 1.0f, // left,  top
//        1.0f, 1.0f, // right, top

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
