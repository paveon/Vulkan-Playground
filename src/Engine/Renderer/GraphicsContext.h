#ifndef VULKAN_GRAPHICSCONTEXT_H
#define VULKAN_GRAPHICSCONTEXT_H

#include <memory>

class GfxContext {
protected:
    GfxContext() = default;

public:
    virtual ~GfxContext();

    virtual void Init() = 0;

    virtual void RecreateSwapchain() = 0;

    virtual auto FramebufferSize() const -> std::pair<uint32_t, uint32_t> = 0;

    static auto Create(void *windowHandle) -> std::unique_ptr<GfxContext>;
};

#endif //VULKAN_GRAPHICSCONTEXT_H
