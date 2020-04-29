#include "FilterManager.h"
#include "RenderNodeList.h"

RenderNodeList::RenderNodeList() {
    head = nullptr;
    tail = nullptr;
    length = 0;
}

RenderNodeList::~RenderNodeList() {
    while (head != nullptr) {
        RenderNode *node = head->nextNode;
        head->destroy();
        delete head;
        head = node;
    }
}

// 节点链表是一个双向链表
bool RenderNodeList::addNode(RenderNode *node) {
    if (node->getNodeType() == NODE_NONE) {
        return false;
    }
    // 如果结点头为空值，则表示此时为空链表，直接插入
    if (!head) {
        node->prevNode = nullptr;
        node->nextNode = head;
        // 只有一个节点的时候头尾都是这个节点
        head = node;
        tail = node;
    } else {
        // 查找后继结点
        RenderNode *tmp = head;
        for (int i = 0; i < length; ++i) {
            // 判断如果结点为空，节点链表的顺序是从小到大的，这里是从链表中找到一个比当前节点大的节点，即当前节点的后继节点
            // 如果到最后链表中的都没有一个节点比当前节点大，那么此时tmp其实是null
            if ((!tmp) || (tmp->getNodeType() > node->getNodeType())) {
                break;
            }
            tmp = tmp->nextNode;
        }

        // 判断找到后继结点，则将结点插入后继结点前面
        if (tmp) {
            node->prevNode = tmp->prevNode;
            // 判断前继结点是否存在，绑定前继结点的后继结点
            if (tmp->prevNode) {
                tmp->prevNode->nextNode = node;
            } else { // 如果前继结点不存在，则表示tmp为头结点，需要更新头结点指针的位置
                head = node;
            }
            node->nextNode = tmp;
            tmp->prevNode = node;
        } else { // 如果后继结点不存在，则直接插到尾结点的后面
            node->prevNode = tail;
            node->nextNode = nullptr;
            tail->nextNode = node;
            tail = node;
        }
    }
    length++;
    return true;
}

RenderNode *RenderNodeList::removeNode(RenderNodeType type) {
    RenderNode *node = findNode(type);
    // 查找到需要删除的渲染结点，从渲染链表中移除
    if (node) {
        // 将前驱结点指向结点的后继结点
        if (node->prevNode) {
            node->prevNode->nextNode = node->nextNode;
        }
        // 将后继结点指向结点的前驱结点
        if (node->nextNode) {
            node->nextNode->prevNode = node->prevNode;
        } else { // 如果不存在后继结点，则表示需要移除的结点是尾结点。需要更新尾结点的位置
            tail = node->prevNode;
        }
        // 断开需要删除的渲染结点的前继和后继结点链接
        node->prevNode = nullptr;
        node->nextNode = nullptr;
        length--;
    }
    return node;
}

RenderNode *RenderNodeList::findNode(RenderNodeType type) {
    RenderNode *node = head;
    // 遍历节点链表，从当前链表的头部开始遍历，如果存在type类型的节点，则返回，否则返回null
    while (node != nullptr && node->getNodeType() != type) {
        node = node->nextNode;
    }
    return node;
}

void RenderNodeList::init() {
    RenderNode *node = head;
    // 初始化节点链表中的所有渲染节点
    while (node != nullptr) {
        node->init();
        node = node->nextNode;
    }
}

void RenderNodeList::setTextureSize(int width, int height) {
    RenderNode *node = head;
    while (node != nullptr) {
        node->setTextureSize(width, height);
        // 创建渲染结点的FBO
        if (node->getNodeType() != NODE_DISPLAY && !node->hasFrameBuffer()) {
            FrameBuffer *frameBuffer = new FrameBuffer(width, height);
            frameBuffer->init();
            // 每个节点都有一个FBO，用来保存节点滤镜的渲染结果
            node->setFrameBuffer(frameBuffer);
        }
        node = node->nextNode;
    }
}

void RenderNodeList::setDisplaySize(int width, int height) {
    RenderNode *node = head;
    // 遍历所有的节点并设置视口的大小
    while (node != nullptr) {
        node->setDisplaySize(width, height);
        node = node->nextNode;
    }
}

void RenderNodeList::setTimeStamp(double timeStamp) {
    RenderNode *node = head;
    while (node != nullptr) {
        node->setTimeStamp(timeStamp);
        node = node->nextNode;
    }
}

void RenderNodeList::setIntensity(float intensity) {
    RenderNode *node = head;
    while (node != nullptr) {
        node->setIntensity(intensity);
        node = node->nextNode;
    }
}

void RenderNodeList::setIntensity(RenderNodeType type, float intensity) {
    RenderNode *node = findNode(type);
    if (node != nullptr) {
        node->setIntensity(intensity);
    }
}

void RenderNodeList::changeFilter(RenderNodeType type, GLFilter *filter) {
    if (type == NODE_NONE) {
        return;
    }
    // 如果存在type类型的渲染结点，则切换当前渲染结点的滤镜filter
    // 否则，创建一个新的渲染结点并添加到渲染链表中
    RenderNode *node = findNode(type);
    if (!node) {
        node = new RenderNode(type);
        addNode(node);
    }
    node->changeFilter(filter);
}

void RenderNodeList::changeFilter(RenderNodeType type, const char *name) {
    if (type == NODE_NONE) {
        return;
    }
    RenderNode *node = findNode(type);
    if (!node) {
        node = new RenderNode(type);
        addNode(node);
    }
    node->changeFilter(FilterManager::getInstance()->getFilter(name));
}

void RenderNodeList::changeFilter(RenderNodeType type, const int id) {
    RenderNode *node = findNode(type);
    if (!node) {
        node = new RenderNode(type);
        addNode(node);
    }
    node->changeFilter(FilterManager::getInstance()->getFilter(id));
}

bool RenderNodeList::drawFrame(GLuint texture, const float *vertices, const float *textureVertices) {
    RenderNode *node = head;
    GLuint currentTexture = texture;
    bool result = true;
    // 遍历所有的渲染节点
    while (node != nullptr) {
        // 如果是尾结点直接绘制输出，一般尾节点是显示节点，因为链表是从小到大连接的，而显示节点是最大的，所有一般显示节点就是尾节点
        if (node == tail) {
            result = node->drawFrame(currentTexture, vertices, textureVertices);
        } else {
            // 否则继续渲染下一个节点
            // 调用节点的drawFrameBuffer，此时又会将渲染结果保存到当前节点的FBO中，返回当前FBO的纹理对象
            currentTexture = node->drawFrameBuffer(currentTexture, vertices, textureVertices);
        }
        node = node->nextNode;
    }
    return result;
}

int RenderNodeList::drawFrameBuffer(GLuint texture, const float *vertices, const float *textureVertices) {
    RenderNode *node = head;
    GLuint currentTexture = texture;
    while (node != nullptr) {
        currentTexture = node->drawFrameBuffer(currentTexture, vertices, textureVertices);
        node = node->nextNode;
    }
    return currentTexture;
}

bool RenderNodeList::isEmpty() {
    return (length == 0);
}

int RenderNodeList::size() {
    return length;
}

void RenderNodeList::deleteAll() {
    RenderNode *node = head;
    while (node != nullptr) {
        head = node->nextNode;
        node->destroy();
        delete node;
        node = head;
    }
    length = 0;
    head = nullptr;
    tail = nullptr;
}

void RenderNodeList::dump() {
    RenderNode *node = head;
    while (node != nullptr) {
        LOGD("dump - node type = %d", node->getNodeType());
        node = node->nextNode;
    }
}
