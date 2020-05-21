#ifndef VULKAN_RENDERPASS_H
#define VULKAN_RENDERPASS_H


#include <memory>
#include <vulkan/vulkan.h>

class RenderPass {
protected:
   RenderPass() = default;

public:
   virtual ~RenderPass() = default;

   static std::unique_ptr<RenderPass> Create();

   virtual auto VkHandle() const -> const void* { return nullptr; }
};


#endif //VULKAN_RENDERPASS_H
