
#include "DisplayRenderNode.h"

DisplayRenderNode::DisplayRenderNode() : RenderNode(NODE_DISPLAY) { // 直接显示渲染节点
    // 输出滤镜
    glFilter = new GLOutFilter();
}
