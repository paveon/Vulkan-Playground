#ifndef VULKAN_RENDERPASSVK_H
#define VULKAN_RENDERPASSVK_H


#include <Engine/Renderer/RenderPass.h>
#include "GraphicsContextVk.h"


class RenderPassVk : public RenderPass {
private:
    VkRenderPass m_RenderPass = nullptr;

    GfxContextVk& m_Context;
    Device& m_Device;

    std::vector<VkAttachmentDescription> m_Attachments;
    std::vector<VkAttachmentReference> m_ColorAttachments;
    std::vector<VkSubpassDescription> m_Subpasses;
    std::vector<VkClearValue> m_ClearValues;

public:
    explicit RenderPassVk(bool test);

    RenderPassVk(std::vector<VkAttachmentDescription> attachments,
                 std::vector<VkSubpassDescription> subpasses,
                 std::vector<VkSubpassDependency> dependencies,
                 const std::vector<uint32_t> &colorAttachmentIndices,
                 std::optional<uint32_t> depthAttachmentIndex);

    ~RenderPassVk() override;

    auto VkHandle() const -> const void* override { return m_RenderPass; };

    auto ptr() const noexcept -> const VkRenderPass * { return &m_RenderPass; }

    auto data() const noexcept -> const VkRenderPass & { return m_RenderPass; }

    auto ColorAttachmentCount() const -> uint32_t { return m_ColorAttachments.size(); }

    void SetClearValue(size_t attachmentIndex, VkClearValue value) {
        assert(attachmentIndex < m_ClearValues.size());
        m_ClearValues[attachmentIndex] = value;
    }

    void Begin(const vk::CommandBuffer& cmdBuffer, const vk::Framebuffer& framebuffer) const;

    static void End(const vk::CommandBuffer& cmdBuffer) { vkCmdEndRenderPass(cmdBuffer.data()); }
};


#endif //VULKAN_RENDERPASSVK_H
