#ifndef VULKAN_GRAPHICSCONTEXTVK_H
#define VULKAN_GRAPHICSCONTEXTVK_H

#include "Engine/Renderer/vulkan_wrappers.h"
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/GraphicsContext.h"

struct GLFWwindow;

class GfxContextVk : public GfxContext {
private:
    GLFWwindow *m_WindowHandle = nullptr;
    vk::Instance m_Instance;
    vk::Surface m_Surface;
    Device m_Device;
    vk::Swapchain* m_Swapchain;

public:
    explicit GfxContextVk(GLFWwindow *windowHandle);

    ~GfxContextVk() override = default;

    void Init() override;

    auto Instance() const -> const vk::Instance & { return m_Instance; }

    auto Surface() const -> const vk::Surface & { return m_Surface; }

    auto Swapchain() -> vk::Swapchain & { return *m_Swapchain; }

    auto GetDevice() -> Device & { return m_Device; }

    auto RecreateSwapchain() -> void override;

    auto FramebufferSize() const -> std::pair<uint32_t, uint32_t> override {
        auto extent = m_Swapchain->Extent();
        return {extent.width, extent.height};
    }

    auto FramebufferSizeVk() const -> VkExtent2D {
        return m_Swapchain->Extent();
    }
};

#endif //VULKAN_GRAPHICSCONTEXTVK_H
