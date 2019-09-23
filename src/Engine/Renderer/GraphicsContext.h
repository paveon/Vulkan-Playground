#ifndef VULKAN_GRAPHICSCONTEXT_H
#define VULKAN_GRAPHICSCONTEXT_H

#include <memory>

class GfxContext {
protected:
    GfxContext() = default;

public:
    virtual void Init() = 0;

    static std::unique_ptr<GfxContext> Create(void* windowHandle);
};

#endif //VULKAN_GRAPHICSCONTEXT_H
