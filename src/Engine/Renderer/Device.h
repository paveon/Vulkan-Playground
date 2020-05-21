#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <tuple>
#include <cstring>

#include "utils.h"
#include "vulkan_wrappers.h"
#include "Texture.h"


class Device {
private:
    const vk::Instance *m_Instance{};
    const vk::Surface *m_Surface{};
    VkPhysicalDevice m_PhysicalDevice = nullptr;
    vk::LogicalDevice m_LogicalDevice;
    VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPhysicalDeviceProperties m_Properties = {};

    std::unique_ptr<vk::Swapchain> m_Swapchain;

    vk::CommandPool* m_GfxCmdPool;
    vk::CommandPool* m_TransferCmdPool;

    std::vector<std::unique_ptr<vk::Pipeline>> m_Pipelines;
    std::vector<std::unique_ptr<vk::PipelineLayout>> m_PipelineLayouts;
    std::vector<std::unique_ptr<vk::PipelineCache>> m_PipelineCaches;

    std::vector<std::unique_ptr<vk::CommandPool>> m_CmdPools;
    std::vector<std::unique_ptr<vk::CommandBuffers>> m_CmdBuffers;
    std::vector<std::unique_ptr<vk::Image>> m_Images;
    std::vector<std::unique_ptr<vk::DeviceMemory>> m_DeviceMemories;
    std::vector<std::unique_ptr<vk::ImageView>> m_ImageViews;
    std::vector<std::unique_ptr<vk::Buffer>> m_Buffers;
    std::vector<std::unique_ptr<vk::Framebuffer>> m_Framebuffers;
    std::vector<std::unique_ptr<vk::Semaphore>> m_Semaphores;
    std::vector<std::unique_ptr<vk::Fence>> m_Fences;

    std::vector<std::unique_ptr<vk::DescriptorPool>> m_DescriptorPools;
    std::vector<std::unique_ptr<vk::DescriptorSets>> m_DescriptorSets;
    std::vector<std::unique_ptr<vk::DescriptorSetLayout>> m_DescriptorSetLayouts;
    std::vector<std::unique_ptr<vk::Sampler>> m_Samplers;
    std::vector<std::unique_ptr<vk::ShaderModule>> m_ShaderModules;

    void Move(Device &other) {
        m_Instance = other.m_Instance;
        m_Surface = other.m_Surface;
        m_PhysicalDevice = other.m_PhysicalDevice;
        m_MsaaSamples = other.m_MsaaSamples;
        m_GfxCmdPool = other.m_GfxCmdPool;
        m_TransferCmdPool = other.m_TransferCmdPool;

        m_LogicalDevice = std::move(other.m_LogicalDevice);
        m_Pipelines = std::move(other.m_Pipelines);
        m_PipelineLayouts = std::move(other.m_PipelineLayouts);
        m_PipelineCaches = std::move(other.m_PipelineCaches);
        m_CmdPools = std::move(other.m_CmdPools);
        m_CmdBuffers = std::move(other.m_CmdBuffers);
        m_Images = std::move(other.m_Images);
        m_DeviceMemories = std::move(other.m_DeviceMemories);
        m_ImageViews = std::move(other.m_ImageViews);
        m_Buffers = std::move(other.m_Buffers);
        m_Framebuffers = std::move(other.m_Framebuffers);
        m_Semaphores = std::move(other.m_Semaphores);
        m_Fences = std::move(other.m_Fences);
        m_DescriptorPools = std::move(other.m_DescriptorPools);
        m_DescriptorSets = std::move(other.m_DescriptorSets);
        m_Samplers = std::move(other.m_Samplers);
        m_ShaderModules = std::move(other.m_ShaderModules);
    }

    void Release();

public:
    ~Device() { Release(); }

    Device(const vk::Instance &instance, const vk::Surface &surface);

    Device(const Device &other) = delete;

    auto operator=(const Device &other) -> Device & = delete;

    Device(Device &&other) noexcept { Move(other); }

    auto operator=(Device &&other) noexcept -> Device & {
        if (this == &other) return *this;
        Release();
        Move(other);
        return *this;
    }

    operator VkPhysicalDevice() const { return m_PhysicalDevice; }

    operator VkDevice() const { return m_LogicalDevice.data(); }

    auto Name() const -> const char * { return m_Properties.deviceName; }

    auto GfxQueue() const -> VkQueue { return m_LogicalDevice.GfxQueue(); }

    auto GfxQueueIdx() const -> uint32_t { return m_LogicalDevice.GfxQueueIdx(); }

    auto PresentQueue() const -> VkQueue { return m_LogicalDevice.PresentQueue(); }

    auto PresentQueueIdx() const -> uint32_t { return m_LogicalDevice.PresentQueueIdx(); }

    auto TransferQueue() const -> VkQueue { return m_LogicalDevice.TransferQueue(); }

    auto TransferQueueIdx() const -> uint32_t { return m_LogicalDevice.TransferQueueIdx(); }

    auto GfxPool() const -> vk::CommandPool* { return m_GfxCmdPool; }

    auto TransferPool() const -> vk::CommandPool* { return m_TransferCmdPool; }


    auto queue(QueueFamily family) const -> VkQueue { return m_LogicalDevice.queue(family); }

    auto queueIndex(QueueFamily family) const -> uint32_t { return m_LogicalDevice.queueIndex(family); }

    static auto maxSamples() -> VkSampleCountFlagBits {
        return VK_SAMPLE_COUNT_1_BIT;
        //return m_MsaaSamples; //TODO
    }

    auto createBuffer(const std::set<uint32_t> &queueIndices, VkDeviceSize bufferSize, VkBufferUsageFlags usage,
                      VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE) -> vk::Buffer * {
        m_Buffers.emplace_back(
                std::make_unique<vk::Buffer>(m_LogicalDevice.data(), queueIndices, bufferSize, usage, sharingMode));
        return m_Buffers.back().get();
    }

    auto allocateBufferMemory(const vk::Buffer &buffer, VkMemoryPropertyFlags properties) -> vk::DeviceMemory * {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_LogicalDevice.data(), buffer.data(), &memRequirements);
        m_DeviceMemories.emplace_back(
                std::make_unique<vk::DeviceMemory>(m_LogicalDevice.data(), findMemoryType(memRequirements, properties),
                                                   memRequirements.size));
        return m_DeviceMemories.back().get();
    }

    auto allocateImageMemory(const vk::Image &image, VkMemoryPropertyFlags properties) -> vk::DeviceMemory * {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_LogicalDevice.data(), image.data(), &memRequirements);

        m_DeviceMemories.emplace_back(
                std::make_unique<vk::DeviceMemory>(m_LogicalDevice.data(), findMemoryType(memRequirements, properties),
                                                   memRequirements.size));
        return m_DeviceMemories.back().get();
    }

    auto createSwapChain(const std::set<uint32_t> &queueIndices, VkExtent2D extent) -> vk::Swapchain *;

    auto createCommandPool(uint32_t queueIndex, VkCommandPoolCreateFlags flags = 0) -> vk::CommandPool * {
        m_CmdPools.emplace_back(std::make_unique<vk::CommandPool>(m_LogicalDevice.data(), queueIndex, flags));
        return m_CmdPools.back().get();
    }

    auto createDescriptorSets(
            const vk::DescriptorPool &pool,
            const std::vector<VkDescriptorSetLayout> &layouts
    ) -> vk::DescriptorSets * {
        m_DescriptorSets.emplace_back(std::make_unique<vk::DescriptorSets>(m_LogicalDevice.data(), pool.data(), layouts));
        return m_DescriptorSets.back().get();
    }

    auto createDescriptorPool(
            const std::vector<VkDescriptorPoolSize> &poolSizes,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags flags = 0
    ) -> vk::DescriptorPool * {
        m_DescriptorPools.emplace_back(
                std::make_unique<vk::DescriptorPool>(m_LogicalDevice.data(), poolSizes, maxSets, flags));
        return m_DescriptorPools.back().get();
    }

    auto createCommandBuffers(uint32_t count) -> vk::CommandBuffers * {
        m_CmdBuffers.emplace_back(
                std::make_unique<vk::CommandBuffers>(m_LogicalDevice.data(), m_GfxCmdPool->data(), count));
        return m_CmdBuffers.back().get();
    }

    auto createCommandBuffers() -> vk::CommandBuffers * {
        return createCommandBuffers(1);
    }

    auto createCommandBuffers(const vk::CommandPool &pool, uint32_t count) -> vk::CommandBuffers * {
        m_CmdBuffers.emplace_back(std::make_unique<vk::CommandBuffers>(m_LogicalDevice.data(), pool.data(), count));
        return m_CmdBuffers.back().get();
    }

    auto createCommandBuffers(const vk::CommandPool &pool) -> vk::CommandBuffers * {
        return createCommandBuffers(pool, 1);
    }

    auto createSemaphore() -> vk::Semaphore * {
        m_Semaphores.emplace_back(std::make_unique<vk::Semaphore>(m_LogicalDevice.data()));
        return m_Semaphores.back().get();
    }

    auto createFence(bool signaled) -> vk::Fence * {
        m_Fences.emplace_back(std::make_unique<vk::Fence>(m_LogicalDevice.data(), signaled));
        return m_Fences.back().get();
    }

    auto createImage(
            const std::set<uint32_t> &queueIndices,
            VkExtent2D extent,
            uint32_t mipLevels,
            VkSampleCountFlagBits samples,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage
    ) -> vk::Image * {
        m_Images.emplace_back(
                std::make_unique<vk::Image>(m_LogicalDevice.data(), queueIndices, extent, mipLevels, samples, format, tiling,
                                            usage));
        return m_Images.back().get();
    }

    auto createImage(const VkImageCreateInfo &createInfo) {
        m_Images.emplace_back(std::make_unique<vk::Image>(m_LogicalDevice.data(), createInfo));
        return m_Images.back().get();
    }

    auto createTextureImage(const std::set<uint32_t> &queueIndices, const Texture2D &texture) -> vk::Image * {
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usageFlags =
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VkImageTiling tilingMode = VK_IMAGE_TILING_OPTIMAL;
        return createImage(queueIndices, {texture.Width(), texture.Height()}, texture.MipLevels(),
                           VK_SAMPLE_COUNT_1_BIT, imageFormat, tilingMode, usageFlags);
    }

    auto createImageView(
            const vk::Image &image,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
    ) -> vk::ImageView * {
        m_ImageViews.emplace_back(std::make_unique<vk::ImageView>(m_LogicalDevice.data(), image, aspectFlags));
        return m_ImageViews.back().get();
    }


    auto createImageView(
            VkImage image,
            uint32_t mipLevels,
            VkFormat format,
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
    ) -> vk::ImageView * {
        m_ImageViews.emplace_back(
                std::make_unique<vk::ImageView>(m_LogicalDevice.data(), image, format, mipLevels, aspectFlags));
        return m_ImageViews.back().get();
    }

    auto createSampler(float maxLod) -> vk::Sampler * {
        m_Samplers.emplace_back(std::make_unique<vk::Sampler>(m_LogicalDevice.data(), maxLod));
        return m_Samplers.back().get();
    }

    auto createShaderModule(const char* filepath) -> vk::ShaderModule* {
        m_ShaderModules.emplace_back(std::make_unique<vk::ShaderModule>(m_LogicalDevice.data(), filepath));
        return m_ShaderModules.back().get();
    }

    auto createFramebuffer(const VkExtent2D &extent, VkRenderPass renderPass,
                           const std::vector<VkImageView> &attachments) -> vk::Framebuffer * {
        m_Framebuffers.emplace_back(
                std::make_unique<vk::Framebuffer>(m_LogicalDevice.data(), renderPass, extent, attachments));
        return m_Framebuffers.back().get();
    }

    auto
    createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding> &bindings) -> vk::DescriptorSetLayout * {
        m_DescriptorSetLayouts.emplace_back(std::make_unique<vk::DescriptorSetLayout>(m_LogicalDevice.data(), bindings));
        return m_DescriptorSetLayouts.back().get();
    }


    auto createPipelineLayout(const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts = {},
                              const std::vector<VkPushConstantRange> &pushConstants = {}
    ) -> vk::PipelineLayout * {
        m_PipelineLayouts.emplace_back(
                std::make_unique<vk::PipelineLayout>(m_LogicalDevice.data(), descriptorSetLayouts, pushConstants));
        return m_PipelineLayouts.back().get();
    }

    auto createPipelineCache(size_t dataSize, const void *data) -> vk::PipelineCache * {
        m_PipelineCaches.emplace_back(std::make_unique<vk::PipelineCache>(m_LogicalDevice.data(), dataSize, data));
        return m_PipelineCaches.back().get();
    }

    auto
    createPipeline(const VkGraphicsPipelineCreateInfo &createInfo, VkPipelineCache cache = nullptr) -> vk::Pipeline * {
        m_Pipelines.emplace_back(std::make_unique<vk::Pipeline>(m_LogicalDevice.data(), createInfo, cache));
        return m_Pipelines.back().get();
    }

    auto findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features) const -> VkFormat;

    auto findDepthFormat() const -> VkFormat {
        return findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }


    auto getSurfaceCapabilities() const -> VkSurfaceCapabilitiesKHR {
        VkSurfaceCapabilitiesKHR tmp;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface->data(), &tmp) != VK_SUCCESS)
            throw std::runtime_error("Could not query surface for capabilities");
        return tmp;
    }

    auto findMemoryType(const VkMemoryRequirements &requirements, VkMemoryPropertyFlags properties) const -> uint32_t;

    auto properties() const -> const VkPhysicalDeviceProperties& { return m_Properties; }

private:
    auto pickPhysicalDevice() -> VkPhysicalDevice;
};


class StagingBuffer {
private:
    Device *m_Device = nullptr;
    vk::Buffer *m_StagingBuffer = nullptr;
    vk::DeviceMemory *m_BufferMemory = nullptr;
    VkDeviceSize m_DataSize = 0;

    void ReleaseData() { m_DataSize = 0; }

    void Move(StagingBuffer &other) {
        m_Device = other.m_Device;
        m_StagingBuffer = other.m_StagingBuffer;
        m_BufferMemory = other.m_BufferMemory;
        m_DataSize = other.m_DataSize;
    }

public:
    StagingBuffer() = default;

    StagingBuffer(Device *device, const void *data, VkDeviceSize dataSize) : m_Device(device) {
        StageData(data, dataSize);
    }

    ~StagingBuffer() { ReleaseData(); }

    StagingBuffer(const StagingBuffer &other) = delete;

    auto operator=(const StagingBuffer &other) -> StagingBuffer & = delete;

    StagingBuffer(StagingBuffer &&other) noexcept { Move(other); }

    auto operator=(StagingBuffer &&other) noexcept -> StagingBuffer & {
        if (this == &other) return *this;
        ReleaseData();
        Move(other);
        return *this;
    }

    void StageData(const void *data, VkDeviceSize dataSize) {
        ReleaseData();
        VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        m_StagingBuffer = m_Device->createBuffer({m_Device->queueIndex(QueueFamily::TRANSFER)}, dataSize,
                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        m_BufferMemory = m_Device->allocateBufferMemory(*m_StagingBuffer, memoryFlags);
        vkBindBufferMemory(*m_Device, m_StagingBuffer->data(), m_BufferMemory->data(), 0);

        void *bufferPtr = nullptr;
        vkMapMemory(*m_Device, m_BufferMemory->data(), 0, dataSize, 0, &bufferPtr);
        memcpy(bufferPtr, data, dataSize);
        vkUnmapMemory(*m_Device, m_BufferMemory->data());
        m_DataSize = dataSize;
    }

    auto data() const -> const vk::Buffer & { return *m_StagingBuffer; }

    auto size() const -> VkDeviceSize { return m_DataSize; }

    operator const vk::Buffer &() const { return data(); }
};


class DeviceBuffer {
private:
    Device *m_Device = nullptr;
    vk::Buffer *m_Buffer = nullptr;
    vk::DeviceMemory *m_BufferMemory = nullptr;
    StagingBuffer m_StagingBuffer;

    void Move(DeviceBuffer &other) {
        m_Device = other.m_Device;
        m_Buffer = other.m_Buffer;
        m_BufferMemory = other.m_BufferMemory;
        m_StagingBuffer = std::move(other.m_StagingBuffer);
    }


public:
    DeviceBuffer(Device *device, const void *data, VkDeviceSize dataSize, const vk::CommandBuffer &cmdBuffer,
                 VkBufferUsageFlags usage) :
            m_Device(device), m_StagingBuffer(device, data, dataSize) {
        VkBufferUsageFlags bufferUsage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        std::set<uint32_t> indices = {
                device->queueIndex(QueueFamily::TRANSFER),
                device->queueIndex(QueueFamily::GRAPHICS)
        };
        m_Buffer = device->createBuffer(indices, dataSize, bufferUsage);
        m_BufferMemory = device->allocateBufferMemory(*m_Buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkBindBufferMemory(*device, m_Buffer->data(), m_BufferMemory->data(), 0);
        copyBuffer(cmdBuffer, m_StagingBuffer, *m_Buffer, dataSize);
    }

    DeviceBuffer() = default;

    DeviceBuffer(const DeviceBuffer &other) = delete;

    auto operator=(const DeviceBuffer &other) -> DeviceBuffer & = delete;

    DeviceBuffer(DeviceBuffer &&other) noexcept { Move(other); }

    auto operator=(DeviceBuffer &&other) noexcept -> DeviceBuffer & {
        if (this == &other) return *this;
        Move(other);
        return *this;
    }

    auto buffer() const -> const VkBuffer& { return m_Buffer->data(); }

    auto bufferPtr() const -> const VkBuffer* { return m_Buffer->ptr(); }

    auto data() const -> const vk::Buffer & { return *m_Buffer; }

    auto size() const -> VkDeviceSize { return m_StagingBuffer.size(); }
};


#endif //VULKAN_DEVICE_H