#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <memory>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <mathlib.h>
#include <vulkan/vulkan.h>
#include <Engine/Window.h>

#include "vulkan_wrappers.h"
#include "Device.h"
#include "Platform/Vulkan/GraphicsContextVk.h"
#include "RenderPass.h"
#include "Pipeline.h"


enum class GraphicsAPI {
    VULKAN = 0
};

class WindowResizeEvent;

class Renderer {
public:
    explicit Renderer(GfxContextVk& ctx);

    ~Renderer();

    static auto GetCurrentAPI() -> GraphicsAPI { return s_API; }

    auto AcquireNextImage() -> uint32_t;

    void DrawFrame(std::vector<VkCommandBuffer> &submitBuffers);

    void RecreateSwapchain();

    auto GetRenderPass() const -> RenderPass& { return *m_RenderPass; }

    auto GetDevice() const -> Device& { return m_Device; }

    auto GetCurrentImageIndex() const -> uint32_t { return m_ImageIndex; }

    auto GetFramebuffer() -> vk::Framebuffer& { return *m_Framebuffers[m_ImageIndex]; }

    auto FramebufferSize() const -> std::pair<uint32_t, uint32_t> { return m_Context.FramebufferSize(); }

    void OnWindowResize(WindowResizeEvent& e);
private:
    static GraphicsAPI s_API;

    const size_t MAX_FRAMES_IN_FLIGHT = 2;

    GfxContextVk& m_Context;
    Device& m_Device;

    vk::CommandPool* m_GfxCmdPool;
    vk::CommandPool* m_TransferCmdPool;
    vk::CommandBuffers* m_GfxCmdBuffers;
    vk::CommandBuffers* m_TransferCmdBuffers;

    vk::DeviceMemory* m_ImageMemory;
    vk::Image* m_ColorImage;
    vk::Image* m_DepthImage;
    vk::ImageView* m_ColorImageView;
    vk::ImageView* m_DepthImageView;

    std::vector<vk::Framebuffer*> m_Framebuffers;

    std::vector<vk::Semaphore*> m_AcquireSemaphores;
    std::vector<vk::Semaphore*> m_ReleaseSemaphores;
    std::vector<vk::Fence*> m_Fences;

    std::unique_ptr<RenderPass> m_RenderPass;

    size_t m_FrameIndex = 0;
    uint32_t m_ImageIndex = 0;

    void createSyncObjects();
};


#endif //VULKAN_RENDERER_H
