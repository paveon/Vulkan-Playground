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


enum class QueueFamily {
   GRAPHICS = 0,
   PRESENT = 1,
   TRANSFER = 2,
   COUNT = 3
};

class Device {
private:
   const vk::Instance* m_Instance;
   const vk::Surface* m_Surface;
   VkPhysicalDevice m_PhysicalDevice = nullptr;
   VkDevice m_LogicalDevice = nullptr;
   VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
   VkPhysicalDeviceProperties m_Properties = {};

   vk::CommandPool m_CommandPool;

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
      m_CommandPool = std::move(other.m_CommandPool);
      other.m_LogicalDevice = nullptr;
   }

   void Release() { vkDestroyDevice(m_LogicalDevice, nullptr); }

public:
   ~Device() { Release(); }

   Device(const vk::Instance& instance, const vk::Surface& surface);

   Device(const Device& other) = delete;

   Device& operator=(const Device& other) = delete;

   Device(Device&& other) noexcept { Move(other); }

   Device& operator=(Device&& other) noexcept {
      if (this == &other) return *this;
      Release();
      Move(other);
      return *this;
   }

   operator VkPhysicalDevice() const { return m_PhysicalDevice; }

   operator VkDevice() const { return m_LogicalDevice; }

   const char* Name() const { return m_Properties.deviceName; }

   VkQueue GfxQueue() const { return m_Queues[static_cast<size_t>(QueueFamily::GRAPHICS)]; }

   uint32_t GfxQueueIdx() const { return m_QueueIndices[static_cast<size_t>(QueueFamily::GRAPHICS)]; }

   VkQueue PresentQueue() const { return m_Queues[static_cast<size_t>(QueueFamily::PRESENT)]; }

   uint32_t PresentQueueIdx() const { return m_QueueIndices[static_cast<size_t>(QueueFamily::PRESENT)]; }

   VkQueue TransferQueue() const { return m_Queues[static_cast<size_t>(QueueFamily::TRANSFER)]; }

   uint32_t TransferQueueIdx() const { return m_QueueIndices[static_cast<size_t>(QueueFamily::TRANSFER)]; }

   VkQueue queue(QueueFamily family) const { return m_Queues[static_cast<size_t>(family)]; }

   uint32_t queueIndex(QueueFamily family) const { return m_QueueIndices[static_cast<size_t>(family)]; }

   VkSampleCountFlagBits maxSamples() const {
      return VK_SAMPLE_COUNT_1_BIT;
      //return m_MsaaSamples; //TODO
   }

   vk::Buffer createBuffer(const std::set<uint32_t>& queueIndices, VkDeviceSize bufferSize, VkBufferUsageFlags usage,
                           VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE) const {
      return vk::Buffer(m_LogicalDevice, queueIndices, bufferSize, usage, sharingMode);
   }

   vk::DeviceMemory allocateBufferMemory(const vk::Buffer& buffer, VkMemoryPropertyFlags properties) const {
      VkMemoryRequirements memRequirements;
      vkGetBufferMemoryRequirements(m_LogicalDevice, buffer.data(), &memRequirements);
      return vk::DeviceMemory(m_LogicalDevice, findMemoryType(memRequirements, properties), memRequirements.size);
   }

   vk::DeviceMemory allocateImageMemory(const vk::Image& image, VkMemoryPropertyFlags properties) const {
      VkMemoryRequirements memRequirements;
      vkGetImageMemoryRequirements(m_LogicalDevice, image.data(), &memRequirements);
      return vk::DeviceMemory(m_LogicalDevice, findMemoryType(memRequirements, properties), memRequirements.size);
   }

   vk::Swapchain createSwapChain(const std::set<uint32_t>& queueIndices, VkExtent2D extent) const;

   vk::CommandPool createCommandPool(uint32_t queueIndex, VkCommandPoolCreateFlags flags = 0) const {
      return vk::CommandPool(m_LogicalDevice, queueIndex, flags);
   }

   vk::DescriptorSets
   createDescriptorSets(const vk::DescriptorPool& pool, const std::vector<VkDescriptorSetLayout>& layouts) const {
      return vk::DescriptorSets(m_LogicalDevice, pool.data(), layouts);
   }

   vk::DescriptorPool createDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets) const {
      return vk::DescriptorPool(m_LogicalDevice, poolSizes, maxSets);
   }

   vk::CommandBuffer createCommandBuffer(const vk::CommandPool& pool) const {
      return vk::CommandBuffer(m_LogicalDevice, pool.data());
   }

    vk::CommandBuffer createCommandBuffer() const {
       return vk::CommandBuffer(m_LogicalDevice, m_CommandPool.data());
    }

   vk::CommandBuffers createCommandBuffers(const vk::CommandPool& pool, uint32_t count) const {
      return vk::CommandBuffers(m_LogicalDevice, pool.data(), count);
   }

   vk::Semaphore createSemaphore() const { return vk::Semaphore(m_LogicalDevice); }

   vk::Fence createFence(bool signaled) const { return vk::Fence(m_LogicalDevice, signaled); }

   vk::Image
   createImage(const std::set<uint32_t>& queueIndices, VkExtent2D extent, uint32_t mipLevels,
               VkSampleCountFlagBits samples, VkFormat format,
               VkImageTiling tiling, VkImageUsageFlags usage) const {
      return vk::Image(m_LogicalDevice, queueIndices, extent, mipLevels, samples, format, tiling, usage);
   }

   vk::Image createTextureImage(const std::set<uint32_t>& queueIndices, const Texture2D& texture) const {
      VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
      VkImageUsageFlags usageFlags =
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
      VkImageTiling tilingMode = VK_IMAGE_TILING_OPTIMAL;
      return createImage(queueIndices, {texture.Width(), texture.Height()}, texture.MipLevels(),
                         VK_SAMPLE_COUNT_1_BIT, imageFormat, tilingMode, usageFlags);
   }

   vk::ImageView
   createImageView(const vk::Image& image, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) const {
      return vk::ImageView(m_LogicalDevice, image, aspectFlags);
   }

   vk::ImageView
   createImageView(VkImage image, uint32_t mipLevels, VkFormat format,
                   VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) const {
      return vk::ImageView(m_LogicalDevice, image, format, mipLevels, aspectFlags);
   }

   vk::Sampler createSampler(float maxLod) const { return vk::Sampler(m_LogicalDevice, maxLod); }

   vk::Framebuffer createFramebuffer(const VkExtent2D& extent, VkRenderPass renderPass,
                                     const std::vector<VkImageView>& attachments) const {
      return vk::Framebuffer(m_LogicalDevice, renderPass, extent, attachments);
   }

   vk::DescriptorSetLayout createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings) const {
      return vk::DescriptorSetLayout(m_LogicalDevice, bindings);
   }

   vk::PipelineLayout
   createGraphicsPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) const {
      return vk::PipelineLayout(m_LogicalDevice, descriptorSetLayouts);
   }

   VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                VkFormatFeatureFlags features) const;

   VkFormat findDepthFormat() const {
      return findSupportedFormat(
              {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
              VK_IMAGE_TILING_OPTIMAL,
              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
      );
   }


   VkSurfaceCapabilitiesKHR getSurfaceCapabilities() const {
      VkSurfaceCapabilitiesKHR tmp;
      if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface->data(), &tmp) != VK_SUCCESS)
         throw std::runtime_error("Could not query surface for capabilities");
      return tmp;
   }


   std::vector<VkSurfaceFormatKHR> getSurfaceFormats() const {
      uint32_t formatCount;
      if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface->data(), &formatCount, nullptr) !=
          VK_SUCCESS)
         throw std::runtime_error("Could not query surface for supported formats");
      std::vector<VkSurfaceFormatKHR> formats(formatCount);
      if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface->data(), &formatCount, formats.data()) !=
          VK_SUCCESS)
         throw std::runtime_error("Could not query surface for supported formats");
      return formats;
   }


   std::vector<VkPresentModeKHR> getSurfacePresentModes() const {
      uint32_t presentModeCount;
      if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface->data(), &presentModeCount, nullptr) !=
          VK_SUCCESS)
         throw std::runtime_error("Could not query surface for supported present modes");
      std::vector<VkPresentModeKHR> modes(presentModeCount);
      if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface->data(), &presentModeCount,
                                                    modes.data()) != VK_SUCCESS)
         throw std::runtime_error("Could not query surface for supported present modes");
      return modes;
   }


private:
   VkPhysicalDevice pickPhysicalDevice();

   void createLogicalDevice();

   uint32_t findMemoryType(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties) const;
};


class StagingBuffer {
private:
   const Device* m_Device = nullptr;
   vk::Buffer m_StagingBuffer;
   vk::DeviceMemory m_BufferMemory;
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

   StagingBuffer(const Device& device, const void* data, VkDeviceSize dataSize) : m_Device(&device) {
      StageData(data, dataSize);
   }

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
      m_StagingBuffer = m_Device->createBuffer({m_Device->queueIndex(QueueFamily::TRANSFER)}, dataSize,
                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
      m_BufferMemory = m_Device->allocateBufferMemory(m_StagingBuffer, memoryFlags);
      vkBindBufferMemory(*m_Device, m_StagingBuffer.data(), m_BufferMemory.data(), 0);

      void* bufferPtr;
      vkMapMemory(*m_Device, m_BufferMemory.data(), 0, dataSize, 0, &bufferPtr);
      memcpy(bufferPtr, data, dataSize);
      vkUnmapMemory(*m_Device, m_BufferMemory.data());
      m_DataSize = dataSize;
   }

   const vk::Buffer& data() const { return m_StagingBuffer; }

   VkDeviceSize size() const { return m_DataSize; }

   operator const vk::Buffer&() const { return data(); }
};


class DeviceBuffer {
private:
    const Device* m_Device = nullptr;
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
    DeviceBuffer(const Device& device, const void* data, VkDeviceSize dataSize, const vk::CommandBuffer& cmdBuffer,
                 VkBufferUsageFlags usage) :
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


#endif //VULKAN_DEVICE_H
