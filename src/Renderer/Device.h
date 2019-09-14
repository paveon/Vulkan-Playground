#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <tuple>

#include "utils.h"
#include "vulkan_wrappers.h"


enum class QueueFamily {
    GRAPHICS = 0,
    PRESENT = 1,
    TRANSFER = 2,
    COUNT = 3
};

class Device {
private:
    VkInstance m_Instance = nullptr;
    VkSurfaceKHR m_Surface = nullptr;
    VkPhysicalDevice m_PhysicalDevice = nullptr;
    VkDevice m_LogicalDevice = nullptr;
    VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkQueue> m_Queues;
    std::vector<uint32_t> m_QueueIndices;

    static const std::array<const char*, 1> s_RequiredExtensions;

    void Move(Device& other) {
       m_Instance = other.m_Instance;
       m_Surface = other.m_Surface;
       m_PhysicalDevice = other.m_PhysicalDevice;
       m_LogicalDevice = other.m_LogicalDevice;
       m_MsaaSamples = other.m_MsaaSamples;
       m_Queues = std::move(other.m_Queues);
       m_QueueIndices = std::move(other.m_QueueIndices);
       other.m_LogicalDevice = nullptr;
    }

    void Release() { vkDestroyDevice(m_LogicalDevice, nullptr); }

public:
    ~Device() { Release(); }

    Device(VkInstance instance, VkSurfaceKHR surface);

    Device(const Device& other) = delete;

    Device& operator=(const Device& other) = delete;

    Device(Device&& other) noexcept { Move(other); }

    Device& operator=(Device&& other) noexcept {
       if (this == &other) return *this;
       Release();
       Move(other);
       return *this;
    }

    operator VkPhysicalDevice() { return m_PhysicalDevice; }

    operator VkDevice() { return m_LogicalDevice; }

    VkQueue queue(QueueFamily family) { return m_Queues[static_cast<size_t>(family)]; }

    uint32_t queueIndex(QueueFamily family) { return m_QueueIndices[static_cast<size_t>(family)]; }

    VkSampleCountFlagBits maxSamples() { return m_MsaaSamples; }

    Buffer createBuffer(const std::set<uint32_t>& queueIndices, VkDeviceSize bufferSize, VkBufferUsageFlags usage,
                        VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE) {
       return Buffer(m_LogicalDevice, queueIndices, bufferSize, usage, sharingMode);
    }

    DeviceMemory allocateBufferMemory(const Buffer& buffer, VkMemoryPropertyFlags properties) {
       VkMemoryRequirements memRequirements;
       vkGetBufferMemoryRequirements(m_LogicalDevice, buffer.data(), &memRequirements);
       return DeviceMemory(m_LogicalDevice, findMemoryType(memRequirements, properties), memRequirements.size);
    }

    DeviceMemory allocateImageMemory(const Image& image, VkMemoryPropertyFlags properties) {
       VkMemoryRequirements memRequirements;
       vkGetImageMemoryRequirements(m_LogicalDevice, image.data(), &memRequirements);
       return DeviceMemory(m_LogicalDevice, findMemoryType(memRequirements, properties), memRequirements.size);
    }

    Swapchain createSwapChain(const std::set<uint32_t>& queueIndices, VkExtent2D extent);

    CommandPool createCommandPool(uint32_t queueIndex, VkCommandPoolCreateFlags flags = 0) {
       return CommandPool(m_LogicalDevice, queueIndex, flags);
    }

    DescriptorSets createDescriptorSets(const DescriptorPool& pool, const std::vector<VkDescriptorSetLayout>& layouts) {
       return DescriptorSets(m_LogicalDevice, pool.data(), layouts);
    }

    DescriptorPool createDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets) {
       return DescriptorPool(m_LogicalDevice, poolSizes, maxSets);
    }

    CommandBuffer createCommandBuffer(const CommandPool& pool) { return CommandBuffer(m_LogicalDevice, pool.data()); }

    CommandBuffers createCommandBuffers(const CommandPool& pool, uint32_t count) { return CommandBuffers(m_LogicalDevice, pool.data(), count); }

    Semaphore createSemaphore() { return Semaphore(m_LogicalDevice); }

    Fence createFence(bool signaled) { return Fence(m_LogicalDevice, signaled); }

    Image createImage(const std::set<uint32_t>& queueIndices, VkExtent2D extent, uint32_t mipLevels, VkSampleCountFlagBits samples, VkFormat format,
                      VkImageTiling tiling, VkImageUsageFlags usage) {
       return Image(m_LogicalDevice, queueIndices, extent, mipLevels, samples, format, tiling, usage);
    }

    Image createTextureImage(const std::set<uint32_t>& queueIndices, const Texture& texture) {
       VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
       VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
       VkImageTiling tilingMode = VK_IMAGE_TILING_OPTIMAL;
       return createImage(queueIndices, texture.Extent(), texture.MipLevels(), VK_SAMPLE_COUNT_1_BIT, imageFormat, tilingMode, usageFlags);
    }

    ImageView createImageView(const Image& image, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) {
       return ImageView(m_LogicalDevice, image, aspectFlags);
    }

    ImageView createImageView(VkImage image, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) {
       return ImageView(m_LogicalDevice, image, format, mipLevels, aspectFlags);
    }

    Sampler createSampler(float maxLod) { return Sampler(m_LogicalDevice, maxLod); }

    Framebuffer createFramebuffer(const VkExtent2D& extent, VkRenderPass renderPass, const std::vector<VkImageView>& attachments) {
       return Framebuffer(m_LogicalDevice, renderPass, extent, attachments);
    }

    VkDescriptorSetLayout createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);

    VkPipelineLayout createGraphicsPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat findDepthFormat() {
       return findSupportedFormat(
               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
               VK_IMAGE_TILING_OPTIMAL,
               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
       );
    }


    VkSurfaceCapabilitiesKHR getSurfaceCapabilities() const {
       VkSurfaceCapabilitiesKHR tmp;
       if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &tmp) != VK_SUCCESS)
          throw std::runtime_error("Could not query surface for capabilities");
       return tmp;
    }


    std::vector<VkSurfaceFormatKHR> getSurfaceFormats() const {
       uint32_t formatCount;
       if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr) != VK_SUCCESS)
          throw std::runtime_error("Could not query surface for supported formats");
       std::vector<VkSurfaceFormatKHR> formats(formatCount);
       if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, formats.data()) != VK_SUCCESS)
          throw std::runtime_error("Could not query surface for supported formats");
       return formats;
    }


    std::vector<VkPresentModeKHR> getSurfacePresentModes() const {
       uint32_t presentModeCount;
       if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr) != VK_SUCCESS)
          throw std::runtime_error("Could not query surface for supported present modes");
       std::vector<VkPresentModeKHR> modes(presentModeCount);
       if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, modes.data()) != VK_SUCCESS)
          throw std::runtime_error("Could not query surface for supported present modes");
       return modes;
    }


private:
    VkPhysicalDevice pickPhysicalDevice();

    void createLogicalDevice();

    uint32_t findMemoryType(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties);
};


class StagingBuffer {
private:
    Device* m_Device = nullptr;
    Buffer m_StagingBuffer;
    DeviceMemory m_BufferMemory;
    VkDeviceSize m_DataSize = 0;

    void ReleaseData() { m_DataSize = 0; }

    void Move(StagingBuffer& other) {
       m_Device = other.m_Device;
       m_StagingBuffer = std::move(other.m_StagingBuffer);
       m_BufferMemory = std::move(other.m_BufferMemory);
       m_DataSize = other.m_DataSize;
    }

public:
    StagingBuffer() = default;

    StagingBuffer(Device& device, const void* data, VkDeviceSize dataSize) : m_Device(&device) { StageData(data, dataSize); }

    ~StagingBuffer() { ReleaseData(); }

    StagingBuffer(const StagingBuffer& other) = delete;

    StagingBuffer& operator=(const StagingBuffer& other) = delete;

    StagingBuffer(StagingBuffer&& other) noexcept { Move(other); }

    StagingBuffer& operator=(StagingBuffer&& other) noexcept {
       if (this == &other) return *this;
       ReleaseData();
       Move(other);
       return *this;
    }

    void StageData(const void* data, VkDeviceSize dataSize) {
       ReleaseData();
       VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
       m_StagingBuffer = m_Device->createBuffer({m_Device->queueIndex(QueueFamily::TRANSFER)}, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
       m_BufferMemory = m_Device->allocateBufferMemory(m_StagingBuffer, memoryFlags);
       vkBindBufferMemory(*m_Device, m_StagingBuffer.data(), m_BufferMemory.data(), 0);

       void* bufferPtr;
       vkMapMemory(*m_Device, m_BufferMemory.data(), 0, dataSize, 0, &bufferPtr);
       memcpy(bufferPtr, data, dataSize);
       vkUnmapMemory(*m_Device, m_BufferMemory.data());
       m_DataSize = dataSize;
    }

    const Buffer& data() const { return m_StagingBuffer; }

    VkDeviceSize size() const { return m_DataSize; }

    operator const Buffer&() const { return data(); }
};


#endif //VULKAN_DEVICE_H
