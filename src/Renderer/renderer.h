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

struct UniformBufferObject {
    alignas(16) math::mat4 model;
    alignas(16) math::mat4 view;
    alignas(16) math::mat4 proj;
};


class DeviceBuffer {
private:
    Device* m_Device = nullptr;
    Buffer m_Buffer;
    DeviceMemory m_BufferMemory;
    StagingBuffer m_StagingBuffer;

    void Move(DeviceBuffer& other) {
       m_Device = other.m_Device;
       m_Buffer = std::move(other.m_Buffer);
       m_BufferMemory = std::move(other.m_BufferMemory);
       m_StagingBuffer = std::move(other.m_StagingBuffer);
    }


public:
    DeviceBuffer(Device& device, const void* data, VkDeviceSize dataSize, const CommandBuffer& cmdBuffer, VkBufferUsageFlags usage) :
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

    const Buffer& data() const { return m_Buffer; }

    VkDeviceSize size() const { return m_StagingBuffer.size(); }
};


class ShaderModule {
public:
    ShaderModule(VkDevice device, const std::string& filename) : owner(device) {
       std::vector<char> shaderCode = readFile(filename);
       codeSize = shaderCode.size();

       VkShaderModuleCreateInfo createInfo = {};
       createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
       createInfo.codeSize = codeSize;
       createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

       if (vkCreateShaderModule(owner, &createInfo, nullptr, &data) != VK_SUCCESS) {
          throw std::runtime_error("failed to create shader module!");
       }
    }

    ~ShaderModule() {
       vkDestroyShaderModule(owner, data, nullptr);
    }

    explicit operator VkShaderModule() const { return data; }

    size_t size() const { return codeSize; }

private:
    VkDevice owner = nullptr;
    VkShaderModule data = nullptr;
    size_t codeSize;
};


class Renderer {
public:
    Renderer(VkInstance instance, std::shared_ptr<Window> window, VkExtent2D extent);

    ~Renderer();

    void DrawFrame();

    void RecreateSwapchain(uint32_t width, uint32_t height);

private:
    VkInstance m_Instance = nullptr;
    std::shared_ptr<Window> m_Window = nullptr;
    Surface m_Surface;
    Device m_Device;
    Swapchain m_Swapchain;

    DescriptorPool m_DescriptorPool;
    DescriptorSets m_DescriptorSets;

    CommandPool m_GraphicsCmdPool;
    CommandPool m_TransferCmdPool;
    CommandBuffers m_GraphicsCmdBuffers;
    CommandBuffers m_TransferCmdBuffers;

    std::unique_ptr<Model> m_Model;
    DeviceBuffer m_VertexBuffer;
    DeviceBuffer m_IndexBuffer;

    Image m_TextureImage;
    ImageView m_TextureImageView;
    DeviceMemory m_TextureImageMemory;
    Sampler m_TextureSampler;

    Image m_DepthImage;
    DeviceMemory m_DepthImageMemory;
    ImageView m_DepthImageView;

    Image m_ColorImage;
    DeviceMemory m_ColorImageMemory;
    ImageView m_ColorImageView;

    std::vector<Buffer> m_UniformBuffers;
    std::vector<DeviceMemory> m_UniformMemory;

    std::vector<Framebuffer> m_SwapchainFramebuffers;

    std::vector<Semaphore> m_AcquireSemaphores;
    std::vector<Semaphore> m_ReleaseSemaphores;
    std::vector<Fence> m_Fences;

    const char* vertShaderPath = "shaders/triangle.vert.spv";
    const char* fragShaderPath = "shaders/triangle.frag.spv";

    VkRenderPass m_RenderPass = nullptr;
    VkDescriptorSetLayout m_DescriptorSetLayout = nullptr;
    VkPipelineLayout m_PipelineLayout = nullptr;
    VkPipeline m_GraphicsPipeline = nullptr;

    const size_t MAX_FRAMES_IN_FLIGHT = 2;

    size_t m_FrameIndex = 0;
    bool m_FramebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int, int) {
       auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
       renderer->m_FramebufferResized = true;
    }

    VkPipeline createGraphicsPipeline(VkPipelineLayout layout, VkRenderPass renderPass);

    VkRenderPass createRenderPass();

    void recordCommandBuffers(const CommandBuffers& cmdBuffers);

    void createSyncObjects();

    void createUniformBuffers(size_t count);

    void updateDescriptorSets(const DescriptorSets& descriptorSets);

    void cleanupSwapchain();

    void updateUniformBuffer(uint32_t currentImage);
};


#endif //VULKAN_RENDERER_H
