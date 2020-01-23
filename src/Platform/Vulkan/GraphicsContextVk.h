#ifndef VULKAN_GRAPHICSCONTEXTVK_H
#define VULKAN_GRAPHICSCONTEXTVK_H

#include "Engine/Renderer/vulkan_wrappers.h"
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/GraphicsContext.h"

struct GLFWwindow;

class GfxContextVk : public GfxContext {
private:
   GLFWwindow* m_WindowHandle = nullptr;
   vk::Instance m_Instance;
   vk::Surface m_Surface;
   Device m_Device;
   vk::Swapchain m_Swapchain;

public:
   explicit GfxContextVk(GLFWwindow* windowHandle);

   void Init() override;

   const vk::Instance& Instance() const { return m_Instance; }

   const vk::Surface& Surface() const { return m_Surface; }

   vk::Swapchain& Swapchain() { return m_Swapchain; }

   const Device& GetDevice() const { return m_Device; }

   std::pair<uint32_t, uint32_t> FramebufferSize() const override {
      int height, width;
      glfwGetFramebufferSize(m_WindowHandle, &width, &height);
      return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
   }

   VkExtent2D FramebufferSizeVk() const {
      int height, width;
      glfwGetFramebufferSize(m_WindowHandle, &width, &height);
      return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
   }
};

#endif //VULKAN_GRAPHICSCONTEXTVK_H
