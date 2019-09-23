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
//#include "Engine/ImGuiLayer.h"

struct UniformBufferObject {
    alignas(16) math::mat4 model;
    alignas(16) math::mat4 view;
    alignas(16) math::mat4 proj;
};


class DeviceBuffer {
private:
    Device* m_Device = nullptr;
    vk::Buffer m_Buffer;
    vk::DeviceMemory m_BufferMemory;
    StagingBuffer m_StagingBuffer;

    void Move(DeviceBuffer& other) {
       m_Device = other.m_Device;
       m_Buffer = std::move(other.m_Buffer);
       m_BufferMemory = std::move(other.m_BufferMemory);
       m_StagingBuffer = std::move(other.m_StagingBuffer);
    }


public:
    DeviceBuffer(Device& device, const void* data, VkDeviceSize dataSize, const vk::CommandBuffer& cmdBuffer, VkBufferUsageFlags usage) :
            m_Device(&device), m_StagingBuffer(device, data, dataSize) {
       VkBufferUsageFlags bufferUsage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
       std::set<uint32_t> indices = {
               device.queueIndex(QueueFamily::TRANSFER),
               device.queueIndex(QueueFamily::GRAPHICS)
       };
       m_Buffer = device.createBuffer(indices, dataSize, bufferUsage);
       m_BufferMemory = device.allocateBufferMemory(m_Buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
       vkBindBufferMemory(device, m_Buffer.data(), m_BufferMemory.data(), 0);
       copyBuffer(cmdBuffer, m_StagingBuffer, m_Buffer, dataSize);
    }

    DeviceBuffer() = default;

    DeviceBuffer(const DeviceBuffer& other) = delete;

    DeviceBuffer& operator=(const DeviceBuffer& other) = delete;

    DeviceBuffer(DeviceBuffer&& other) noexcept { Move(other); }

    DeviceBuffer& operator=(DeviceBuffer&& other) noexcept {
       if (this == &other) return *this;
       Move(other);
       return *this;
    }

    const vk::Buffer& data() const { return m_Buffer; }

    VkDeviceSize size() const { return m_StagingBuffer.size(); }
};

enum class GraphicsAPI {
    VULKAN = 0
};

class Renderer {
public:
    Renderer(const GfxContextVk& ctx);

    ~Renderer();

    static GraphicsAPI GetCurrentAPI() { return s_API; }

    void DrawFrame();

    void RecreateSwapchain(uint32_t width, uint32_t height);

    float frameTimer = 1.0f;
    uint32_t frameCounter = 0;

private:
    static GraphicsAPI s_API;

    const Device& m_Device;
    vk::Swapchain m_Swapchain;

    //std::unique_ptr<ImGuiLayer> m_ImGui;

    vk::DescriptorPool m_DescriptorPool;
    vk::DescriptorSets m_DescriptorSets;

    vk::CommandPool m_GraphicsCmdPool;
    vk::CommandPool m_TransferCmdPool;
    vk::CommandBuffers m_GraphicsCmdBuffers;
    vk::CommandBuffers m_TransferCmdBuffers;

    std::unique_ptr<Model> m_Model;
    DeviceBuffer m_VertexBuffer;
    DeviceBuffer m_IndexBuffer;

    vk::Image m_TextureImage;
    vk::ImageView m_TextureImageView;
    vk::DeviceMemory m_TextureImageMemory;
    vk::Sampler m_TextureSampler;

    vk::Image m_DepthImage;
    vk::DeviceMemory m_DepthImageMemory;
    vk::ImageView m_DepthImageView;

    vk::Image m_ColorImage;
    vk::DeviceMemory m_ColorImageMemory;
    vk::ImageView m_ColorImageView;

    std::vector<vk::Buffer> m_UniformBuffers;
    std::vector<vk::DeviceMemory> m_UniformMemory;

    std::vector<vk::Framebuffer> m_SwapchainFramebuffers;

    std::vector<vk::Semaphore> m_AcquireSemaphores;
    std::vector<vk::Semaphore> m_ReleaseSemaphores;
    std::vector<vk::Fence> m_Fences;

    const char* vertShaderPath = "shaders/triangle.vert.spv";
    const char* fragShaderPath = "shaders/triangle.frag.spv";

    VkRenderPass m_RenderPass = nullptr;
    vk::DescriptorSetLayout m_DescriptorSetLayout;
    vk::PipelineLayout m_PipelineLayout;
    vk::Pipeline m_GraphicsPipeline;

    const size_t MAX_FRAMES_IN_FLIGHT = 2;

    size_t m_FrameIndex = 0;
    bool m_FramebufferResized = false;

    vk::Pipeline createGraphicsPipeline(VkPipelineLayout layout, VkRenderPass renderPass);

    VkRenderPass createRenderPass();

    void recordCommandBuffers(const vk::CommandBuffers& cmdBuffers);

    void recordCommandBuffer(VkCommandBuffer cmdBuffer, size_t index);

    void createSyncObjects();

    void createUniformBuffers(size_t count);

    void updateDescriptorSets(const vk::DescriptorSets& descriptorSets);

    void cleanupSwapchain();

    void updateUniformBuffer(uint32_t currentImage);

//    void prepareImGui(VkExtent2D, VkRenderPass, VkQueue) {
////       m_ImGui = std::make_unique<ImGuiLayer>(m_Device);
////       m_ImGui->init(extent.width, extent.height);
////       m_ImGui->initResources(renderPass, queue);
//    }
};


#endif //VULKAN_RENDERER_H
