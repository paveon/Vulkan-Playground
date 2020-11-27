#ifndef VULKAN_RENDERPASSVK_H
#define VULKAN_RENDERPASSVK_H


#include <Engine/Renderer/RenderPass.h>
#include "GraphicsContextVk.h"


class RenderPassVk : public RenderPass {
private:
    VkRenderPass m_RenderPass = nullptr;

    GfxContextVk& m_Context;
    Device& m_Device;

public:
    explicit RenderPassVk(bool test);

    ~RenderPassVk() override;

    auto VkHandle() const -> const void* override { return m_RenderPass; };

    auto ptr() const noexcept -> const VkRenderPass * { return &m_RenderPass; }

    auto data() const noexcept -> const VkRenderPass & { return m_RenderPass; }
};


#endif //VULKAN_RENDERPASSVK_H
