
#ifndef FILTERMANAGER_H
#define FILTERMANAGER_H

#include <mutex>
#include <GLFilter.h>
#include <NodeType.h>

/**
 * 滤镜对象信息
 */
typedef struct FilterInfo {
    RenderNodeType type;
    const char *name;
    int id;
} FilterInfo;


class FilterManager {
public:

    static FilterManager *getInstance();

    void destroy();

    // 根据滤镜信息对象获取滤镜
    GLFilter *getFilter(FilterInfo *filterInfo);

    // 根据名称获取滤镜
    GLFilter *getFilter(const char *name);

    // 根据id来获取滤镜
    GLFilter *getFilter(const int id);

private:
    FilterManager();

    virtual ~FilterManager();

    static FilterManager *instance;
    static std::mutex mutex;
};


#endif //FILTERMANAGER_H
