#ifndef VULKAN_RENDERPASSVK_H
#define VULKAN_RENDERPASSVK_H


#include <Engine/Renderer/RenderPass.h>


class RenderPassVk : public RenderPass {
private:
    VkRenderPass m_RenderPass = nullptr;

public:
    RenderPassVk();

    ~RenderPassVk() override;

    auto VkHandle() const -> const void* override { return m_RenderPass; };

    auto ptr() const noexcept -> const VkRenderPass * { return &m_RenderPass; }

    auto data() const noexcept -> const VkRenderPass & { return m_RenderPass; }
};


#endif //VULKAN_RENDERPASSVK_H
