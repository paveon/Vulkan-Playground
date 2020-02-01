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
#include "Texture.h"


struct UniformBufferObject {
    alignas(16) math::mat4 model;
    alignas(16) math::mat4 view;
    alignas(16) math::mat4 proj;
};


enum class GraphicsAPI {
    VULKAN = 0
};

class ImGuiLayer;

class Renderer {
public:
    Renderer(GfxContextVk& ctx);

    ~Renderer();

    static GraphicsAPI GetCurrentAPI() { return s_API; }

    void AcquireNextImage();

    void DrawFrame();

    void RecreateSwapchain(uint32_t width, uint32_t height);

    RenderPass& GetRenderPass() const { return *m_RenderPass; }

    const Device& GetDevice() const { return m_Device; }

    uint32_t GetCurrentImageIndex() const { return m_ImageIndex; }

    std::pair<uint32_t, uint32_t> FramebufferSize() const { return m_Context.FramebufferSize(); }

    ImGuiLayer* m_ImGui = nullptr;
private:
    static GraphicsAPI s_API;

    const char* vertShaderPath = "shaders/triangle.vert.spv";
    const char* fragShaderPath = "shaders/triangle.frag.spv";
    const size_t MAX_FRAMES_IN_FLIGHT = 2;

    GfxContext& m_Context;

    const Device& m_Device;
    vk::Swapchain& m_Swapchain;

    vk::CommandPool m_GraphicsCmdPool;
    vk::CommandPool m_TransferCmdPool;
    vk::CommandBuffers m_GraphicsCmdBuffers;
    vk::CommandBuffers m_TransferCmdBuffers;

    std::unique_ptr<Model> m_Model;
    DeviceBuffer m_VertexBuffer;
    DeviceBuffer m_IndexBuffer;

    std::unique_ptr<Texture2D> m_Texture;

    vk::Image m_ColorImage;
    vk::DeviceMemory m_ColorImageMemory;
    vk::ImageView m_ColorImageView;

    vk::Image m_DepthImage;
    vk::DeviceMemory m_DepthImageMemory;
    vk::ImageView m_DepthImageView;

    std::vector<vk::Buffer> m_UniformBuffers;
    std::vector<vk::DeviceMemory> m_UniformMemory;

    std::vector<vk::Framebuffer> m_SwapchainFramebuffers;

    std::vector<vk::Semaphore> m_AcquireSemaphores;
    std::vector<vk::Semaphore> m_ReleaseSemaphores;
    std::vector<vk::Fence> m_Fences;

    std::unique_ptr<RenderPass> m_RenderPass;
    std::unique_ptr<Pipeline> m_GraphicsPipeline;

    size_t m_FrameIndex = 0;
    uint32_t m_ImageIndex = 0;

    bool m_FramebufferResized = false;

    void recordCommandBuffer(VkCommandBuffer cmdBuffer, size_t index);

    void createSyncObjects();

    void createUniformBuffers(size_t count);

    void cleanupSwapchain();

    void updateUniformBuffer(uint32_t currentImage);
};


#endif //VULKAN_RENDERER_H
