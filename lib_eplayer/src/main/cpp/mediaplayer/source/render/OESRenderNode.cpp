
#include <GLOESInputFilter.h>
#include "OESRenderNode.h"

OESRenderNode::OESRenderNode() : RenderNode(NODE_INPUT) {
    glFilter = new GLOESInputFilter();
}

OESRenderNode::~OESRenderNode() {

}

void OESRenderNode::updateTransformMatrix(const float *matrix) {
    if (glFilter) {
        ((GLOESInputFilter *) glFilter)->updateTransformMatrix(matrix);
    }
}
