#ifndef VULKAN_RENDERPASSVK_H
#define VULKAN_RENDERPASSVK_H


#include <Engine/Renderer/RenderPass.h>


class RenderPassVk : public RenderPass {
private:
   VkRenderPass m_RenderPass = nullptr;

public:
   RenderPassVk();

   ~RenderPassVk() override;

   VkRenderPass& GetVkHandle() override { return m_RenderPass; };

   const VkRenderPass* ptr() const noexcept { return &m_RenderPass; }

   const VkRenderPass& data() const noexcept { return m_RenderPass; }
};



#endif //VULKAN_RENDERPASSVK_H
