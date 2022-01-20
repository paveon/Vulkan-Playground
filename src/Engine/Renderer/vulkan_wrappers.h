#ifndef VULKAN_VULKAN_WRAPPERS_H
#define VULKAN_VULKAN_WRAPPERS_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <set>
#include <vector>
#include <array>
#include <mathlib.h>
#include <sstream>
#include <map>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include "ShaderResources.h"
#include "Platform/Vulkan/CoreVk.h"


typedef struct GLFWwindow GLFWwindow;


enum class QueueFamily {
    GRAPHICS = 0,
    PRESENT = 1,
    TRANSFER = 2,
    COMPUTE = 3,
    COUNT = 4
};


namespace vk {
    class LogicalDevice {
        VkPhysicalDevice m_Physical = nullptr;
        VkDevice m_Device = nullptr;
        VkSurfaceKHR m_Surface = nullptr;
        VkPhysicalDeviceFeatures m_EnabledFeatures{};

        std::vector<VkQueue> m_Queues;
        std::vector<uint32_t> m_QueueIndices;

        void Move(LogicalDevice &other) noexcept {
           m_Physical = other.m_Physical;
           m_Device = other.m_Device;
           m_Surface = other.m_Surface;
           m_Queues = std::move(other.m_Queues);
           m_QueueIndices = std::move(other.m_QueueIndices);

           other.m_Physical = nullptr;
           other.m_Device = nullptr;
           other.m_Surface = nullptr;
        }

        void Release() noexcept { if (m_Device) vkDestroyDevice(m_Device, nullptr); }

        static const std::array<const char *, 4> s_RequiredExtensions;

    public:
        ~LogicalDevice() { Release(); }

        LogicalDevice() = default;

        LogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

        LogicalDevice(const LogicalDevice &other) = delete;

        auto operator=(const LogicalDevice &other) -> LogicalDevice & = delete;

        LogicalDevice(LogicalDevice &&other) noexcept { Move(other); }

        auto operator=(LogicalDevice &&other) noexcept -> LogicalDevice & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkDevice * { return &m_Device; }

        auto data() const noexcept -> const VkDevice & { return m_Device; }

        auto GfxQueue() const -> VkQueue { return m_Queues[static_cast<size_t>(QueueFamily::GRAPHICS)]; }

        auto GfxQueueIdx() const -> uint32_t { return m_QueueIndices[static_cast<size_t>(QueueFamily::GRAPHICS)]; }

        auto ComputeQueue() const -> VkQueue { return m_Queues[static_cast<size_t>(QueueFamily::COMPUTE)]; }

        auto ComputeQueueIdx() const -> uint32_t { return m_QueueIndices[static_cast<size_t>(QueueFamily::COMPUTE)]; }

        auto PresentQueue() const -> VkQueue { return m_Queues[static_cast<size_t>(QueueFamily::PRESENT)]; }

        auto PresentQueueIdx() const -> uint32_t { return m_QueueIndices[static_cast<size_t>(QueueFamily::PRESENT)]; }

        auto TransferQueue() const -> VkQueue { return m_Queues[static_cast<size_t>(QueueFamily::TRANSFER)]; }

        auto TransferQueueIdx() const -> uint32_t { return m_QueueIndices[static_cast<size_t>(QueueFamily::TRANSFER)]; }

        auto queue(QueueFamily family) const -> VkQueue { return m_Queues[static_cast<size_t>(family)]; }

        auto queueIndex(QueueFamily family) const -> uint32_t { return m_QueueIndices[static_cast<size_t>(family)]; }

        auto getSurfaceFormats() const -> std::vector<VkSurfaceFormatKHR>;

        auto getSurfacePresentModes() const -> std::vector<VkPresentModeKHR>;

        auto enabledFeatures() const -> const VkPhysicalDeviceFeatures & {
           return m_EnabledFeatures;
        }
    };


    class CommandBuffer {
    private:
        VkCommandBuffer m_Buffer = nullptr;

    public:
        explicit CommandBuffer(VkCommandBuffer buffer) : m_Buffer(buffer) {};

        CommandBuffer(const CommandBuffer &other) = default;

        auto operator=(const CommandBuffer &other) -> CommandBuffer & = default;

        CommandBuffer(CommandBuffer &&other) = delete;

        auto operator=(CommandBuffer &&other) = delete;

        auto ptr() const noexcept -> const VkCommandBuffer * { return &m_Buffer; }

        auto data() const noexcept -> const VkCommandBuffer & { return m_Buffer; }

        void Begin(VkCommandBufferBeginInfo beginInfo) const {
           if (vkBeginCommandBuffer(m_Buffer, &beginInfo) != VK_SUCCESS)
              throw std::runtime_error("failed to begin command buffer recording!");
        }

        void Begin(VkCommandBufferUsageFlags flags = 0) const {
           VkCommandBufferBeginInfo beginInfo = {};
           beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
           beginInfo.flags = flags;

           Begin(beginInfo);
        }

        void End() const {
           if (vkEndCommandBuffer(m_Buffer) != VK_SUCCESS)
              throw std::runtime_error("failed to end command buffer recording!");
        }

        void Submit(VkQueue cmdQueue) const {
           VkSubmitInfo submitInfo = {};
           submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
           Submit(submitInfo, cmdQueue);
        }

        void Submit(VkSubmitInfo &submitInfo, VkQueue cmdQueue, VkFence fence = nullptr) const {
           submitInfo.commandBufferCount = 1;
           submitInfo.pCommandBuffers = &m_Buffer;
           VkResult result = vkQueueSubmit(cmdQueue, 1, &submitInfo, fence);
           if (result != VK_SUCCESS) {
              std::ostringstream msg;
              msg << "[vkQueueSubmit] Failed to submit draw command buffer, error code: '" << result << "'";
              throw std::runtime_error(msg.str().c_str());
           }
        }
    };


    class CommandBuffers {
    private:
        VkDevice m_Device = nullptr;
        VkCommandPool m_Pool = nullptr;
        std::vector<VkCommandBuffer> m_Buffers;

        void Move(CommandBuffers &other) noexcept {
           m_Device = other.m_Device;
           m_Pool = other.m_Pool;
           m_Buffers = std::move(other.m_Buffers);
           other.m_Device = nullptr;
           other.m_Pool = nullptr;
           other.m_Buffers.clear();
        }

        void Release() noexcept {
           if (!m_Buffers.empty())
              vkFreeCommandBuffers(m_Device, m_Pool, m_Buffers.size(), m_Buffers.data());
        }

    public:
        ~CommandBuffers() { Release(); }

        CommandBuffers() = default;

        CommandBuffers(VkDevice device,
                       VkCommandPool pool) : CommandBuffers(device, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1) {}

        CommandBuffers(VkDevice device,
                       VkCommandPool pool,
                       VkCommandBufferLevel level) : CommandBuffers(device, pool, level, 1) {}

        CommandBuffers(VkDevice device,
                       VkCommandPool pool,
                       VkCommandBufferLevel level,
                       uint32_t count) : m_Device(device), m_Pool(pool) {
           VkCommandBufferAllocateInfo allocInfo = {};
           allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
           allocInfo.commandPool = m_Pool;
           allocInfo.level = level;
           allocInfo.commandBufferCount = count;

           m_Buffers.resize(count);
           if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_Buffers.data()) != VK_SUCCESS)
              throw std::runtime_error("failed to allocate command buffers!");
        }

        CommandBuffers(const CommandBuffers &other) = delete;

        auto operator=(const CommandBuffers &other) -> CommandBuffers & = delete;

        CommandBuffers(CommandBuffers &&other) noexcept { Move(other); }

        auto operator=(CommandBuffers &&other) noexcept -> CommandBuffers & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        }

        auto operator[](size_t index) const -> CommandBuffer { return get(index); }

        auto get(size_t index) const -> CommandBuffer {
           if (index >= m_Buffers.size()) throw std::runtime_error("out of bounds command buffer indexing");
           return CommandBuffer(m_Buffers[index]);
        }

        auto data() const -> const VkCommandBuffer * { return m_Buffers.data(); }

        auto size() const -> uint32_t { return m_Buffers.size(); }
    };


    class Semaphore {
    private:
        VkSemaphore m_Semaphore = nullptr;
        VkDevice m_Device = nullptr;

        void Move(Semaphore &other) noexcept {
           m_Semaphore = other.m_Semaphore;
           m_Device = other.m_Device;
           other.m_Semaphore = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Semaphore) vkDestroySemaphore(m_Device, m_Semaphore, nullptr); }

    public:
        ~Semaphore() { Release(); }

        Semaphore() = default;

        explicit Semaphore(VkDevice device) : m_Device(device) {
           VkSemaphoreCreateInfo semaphoreInfo = {};
           semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
           if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_Semaphore) != VK_SUCCESS)
              throw std::runtime_error("failed to create a semaphore!");
        }

        Semaphore(const Semaphore &other) = delete;

        auto operator=(const Semaphore &other) -> Semaphore & = delete;

        Semaphore(Semaphore &&other) noexcept { Move(other); }

        auto operator=(Semaphore &&other) noexcept -> Semaphore & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkSemaphore * { return &m_Semaphore; }

        auto data() const noexcept -> const VkSemaphore & { return m_Semaphore; }
    };


    class Fence {
    private:
        VkFence m_Fence = nullptr;
        VkDevice m_Device = nullptr;

        void Move(Fence &other) noexcept {
           m_Fence = other.m_Fence;
           m_Device = other.m_Device;
           other.m_Fence = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Fence) vkDestroyFence(m_Device, m_Fence, nullptr); }

    public:
        ~Fence() { Release(); }

        Fence() = default;

        Fence(VkDevice device, bool signaled) : m_Device(device) {
           VkFenceCreateInfo fenceInfo = {};
           fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
           if (signaled) fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
           if (vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Fence) != VK_SUCCESS)
              throw std::runtime_error("failed to create a fence!");
        }

        Fence(const Fence &other) = delete;

        auto operator=(const Fence &other) -> Fence & = delete;

        Fence(Fence &&other) noexcept { Move(other); }

        auto operator=(Fence &&other) noexcept -> Fence & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkFence * { return &m_Fence; }

        auto data() const noexcept -> const VkFence & { return m_Fence; }
    };


    class CommandPool {
    private:
        VkCommandPool m_Pool = nullptr;
        VkDevice m_Device = nullptr;

        void Move(CommandPool &other) noexcept {
           m_Pool = other.m_Pool;
           m_Device = other.m_Device;
           other.m_Pool = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Pool) vkDestroyCommandPool(m_Device, m_Pool, nullptr); }

    public:
        ~CommandPool() { Release(); }

        CommandPool() = default;

        CommandPool(VkDevice device, uint32_t queueIndex, VkCommandPoolCreateFlags flags = 0) : m_Device(device) {
           VkCommandPoolCreateInfo poolInfo = {};
           poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
           poolInfo.queueFamilyIndex = queueIndex;
           poolInfo.flags = flags;

           if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
              throw std::runtime_error("failed to create command pool!");
        }

        CommandPool(const CommandPool &other) = delete;

        auto operator=(const CommandPool &other) -> CommandPool & = delete;

        CommandPool(CommandPool &&other) noexcept { Move(other); }

        auto operator=(CommandPool &&other) noexcept -> CommandPool & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkCommandPool * { return &m_Pool; }

        auto data() const noexcept -> const VkCommandPool & { return m_Pool; }
    };


    class Framebuffer {
    private:
        VkFramebuffer m_Framebuffer = nullptr;
        VkDevice m_Device = nullptr;
        VkExtent2D m_Extent{};

        void Move(Framebuffer &other) noexcept {
           m_Framebuffer = other.m_Framebuffer;
           m_Device = other.m_Device;
           m_Extent = other.m_Extent;
           other.m_Framebuffer = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Framebuffer) vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr); }

    public:
        ~Framebuffer() { Release(); }

        Framebuffer() = default;

        Framebuffer(VkDevice device,
                    VkRenderPass renderPass,
                    const VkExtent2D &extent,
                    const std::vector<VkImageView> &attachments) :
                m_Device(device), m_Extent(extent) {
           VkFramebufferCreateInfo framebufferInfo = {};
           framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
           framebufferInfo.renderPass = renderPass;
           framebufferInfo.width = m_Extent.width;
           framebufferInfo.height = m_Extent.height;
           framebufferInfo.layers = 1;
           framebufferInfo.attachmentCount = attachments.size();
           framebufferInfo.pAttachments = attachments.data();

           if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS)
              throw std::runtime_error("failed to create framebuffer!");
        }

        Framebuffer(const Framebuffer &other) = delete;

        auto operator=(const Framebuffer &other) -> Framebuffer & = delete;

        Framebuffer(Framebuffer &&other) noexcept { Move(other); }

        auto operator=(Framebuffer &&other) noexcept -> Framebuffer & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkFramebuffer * { return &m_Framebuffer; }

        auto data() const noexcept -> const VkFramebuffer & { return m_Framebuffer; }

        auto Extent() const noexcept -> const VkExtent2D & { return m_Extent; }
    };

    class Image;

    class ImageView {
    private:
        VkImageView m_ImageView = nullptr;
        VkDevice m_Device = nullptr;

        void Move(ImageView &other) noexcept {
           m_ImageView = other.m_ImageView;
           m_Device = other.m_Device;
           other.m_ImageView = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_ImageView) vkDestroyImageView(m_Device, m_ImageView, nullptr); }

    public:
        ~ImageView() { Release(); }

        ImageView() = default;

        ImageView(VkDevice device, const VkImageViewCreateInfo &createInfo) : m_Device(device) {
           if (vkCreateImageView(m_Device, &createInfo, nullptr, &m_ImageView) != VK_SUCCESS)
              throw std::runtime_error("failed to create image views!");
        }

        ImageView(VkDevice device, VkImage image, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspectFlags)
                : m_Device(device) {
           VkImageViewCreateInfo createInfo = {};
           createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
           createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
           createInfo.format = format;
           createInfo.image = image;
           createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
           createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
           createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
           createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
           createInfo.subresourceRange.aspectMask = aspectFlags;
           createInfo.subresourceRange.baseMipLevel = 0;
           createInfo.subresourceRange.levelCount = mipLevels;
           createInfo.subresourceRange.baseArrayLayer = 0;
           createInfo.subresourceRange.layerCount = 1;

           if (vkCreateImageView(m_Device, &createInfo, nullptr, &m_ImageView) != VK_SUCCESS)
              throw std::runtime_error("failed to create image views!");
        }

        ImageView(VkDevice, const Image &image, VkImageAspectFlags aspectFlags);

        ImageView(const ImageView &other) = delete;

        auto operator=(const ImageView &other) -> ImageView & = delete;

        ImageView(ImageView &&other) noexcept { Move(other); }

        auto operator=(ImageView &&other) noexcept -> ImageView & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkImageView * { return &m_ImageView; }

        auto data() const noexcept -> const VkImageView & { return m_ImageView; }
    };


    class Image {
    private:
        friend class ImageView;

        VkImage m_Image = nullptr;
        VkDevice m_Device = nullptr;
        VkImageCreateInfo m_Info{};
        VkMemoryRequirements m_MemoryInfo{};

        void Move(Image &other) noexcept {
           m_Image = other.m_Image;
           m_Device = other.m_Device;
           m_Info = other.m_Info;
           m_MemoryInfo = other.m_MemoryInfo;
           other.m_Image = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Image) vkDestroyImage(m_Device, m_Image, nullptr); }

        void CreateImage(const VkImageCreateInfo &createInfo) {
           m_Info = createInfo;

           if (vkCreateImage(m_Device, &createInfo, nullptr, &m_Image) != VK_SUCCESS)
              throw std::runtime_error("failed to create image!");

           vkGetImageMemoryRequirements(m_Device, m_Image, &m_MemoryInfo);
        }

    public:
        ~Image() { Release(); }

        Image() = default;

        Image(VkDevice device,
              const std::set<uint32_t> &queueIndices,
              const VkExtent2D &extent, uint32_t mipLevels,
              VkSampleCountFlagBits samples,
              VkFormat format,
              VkImageTiling tiling,
              VkImageUsageFlags usage) : m_Device(device) {

           VkImageCreateInfo imageInfo = {};
           imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
           imageInfo.imageType = VK_IMAGE_TYPE_2D;
           imageInfo.extent.width = extent.width;
           imageInfo.extent.height = extent.height;
           imageInfo.extent.depth = 1;
           imageInfo.mipLevels = mipLevels;
           imageInfo.arrayLayers = 1;
           imageInfo.format = format;
           imageInfo.tiling = tiling;
           imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
           imageInfo.usage = usage;
           imageInfo.samples = samples;
           imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

           std::vector<uint32_t> uniqueIndices(queueIndices.begin(), queueIndices.end());
           if (queueIndices.size() < 2) {
              imageInfo.queueFamilyIndexCount = 0;
              imageInfo.pQueueFamilyIndices = nullptr;
           } else {
              imageInfo.queueFamilyIndexCount = uniqueIndices.size();
              imageInfo.pQueueFamilyIndices = uniqueIndices.data();
           }

           CreateImage(imageInfo);
        }

        Image(VkDevice device, const VkImageCreateInfo &createInfo) : m_Device(device) {
           CreateImage(createInfo);
        }

        Image(const Image &other) = delete;

        auto operator=(const Image &other) -> Image & = delete;

        Image(Image &&other) noexcept { Move(other); }

        auto operator=(Image &&other) noexcept -> Image & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkImage * { return &m_Image; }

        auto data() const noexcept -> const VkImage & { return m_Image; }

        auto MipLevels() const -> uint32_t { return m_Info.mipLevels; }

        VkImageMemoryBarrier GetBaseBarrier(VkImageLayout srcLayout,
                                            VkImageLayout dstLayout,
                                            VkAccessFlags srcAccess,
                                            VkAccessFlags dstAccess) {
           return VkImageMemoryBarrier{
                   VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                   nullptr,
                   srcAccess,
                   dstAccess,
                   srcLayout,
                   dstLayout,
                   VK_QUEUE_FAMILY_IGNORED,
                   VK_QUEUE_FAMILY_IGNORED,
                   m_Image,
                   {
                           VK_IMAGE_ASPECT_COLOR_BIT,
                           0,
                           1,
                           0,
                           1
                   }
           };
        }

        void ChangeLayout(const CommandBuffer &cmdBuffer,
                          VkImageLayout srcLayout,
                          VkImageLayout dstLayout,
                          VkPipelineStageFlags srcStages,
                          VkPipelineStageFlags dstStages,
                          VkAccessFlags srcMask,
                          VkAccessFlags dstMask,
                          const std::vector<VkImageSubresourceRange> &subresources);

        void GenerateMipmaps(VkPhysicalDevice physicalDevice, const CommandBuffer &cmdBuffer);

        auto MemoryInfo() const -> const VkMemoryRequirements & { return m_MemoryInfo; }

        auto Info() const -> const auto & { return m_Info; }

        auto Extent() const -> const VkExtent3D & { return m_Info.extent; }

        void BindMemory(VkDeviceMemory memory, VkDeviceSize offset) {
           if (vkBindImageMemory(m_Device, m_Image, memory, offset) != VK_SUCCESS)
              throw std::runtime_error("failed to bind memory to image!");
        }
    };


    inline ImageView::ImageView(VkDevice device, const Image &image, VkImageAspectFlags aspectFlags) : m_Device(
            device) {
       VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
       if (image.m_Info.arrayLayers == 6) {
          viewType = VK_IMAGE_VIEW_TYPE_CUBE;
       }

       VkImageViewCreateInfo createInfo = {};
       createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
       createInfo.viewType = viewType;
       createInfo.format = image.Info().format;
       createInfo.image = image.data();
       createInfo.subresourceRange.aspectMask = aspectFlags;
       createInfo.subresourceRange.baseMipLevel = 0;
       createInfo.subresourceRange.levelCount = image.Info().mipLevels;
       createInfo.subresourceRange.baseArrayLayer = 0;
       createInfo.subresourceRange.layerCount = image.Info().arrayLayers;

       if (vkCreateImageView(m_Device, &createInfo, nullptr, &m_ImageView) != VK_SUCCESS)
          throw std::runtime_error("failed to create image views!");
    }


    class Buffer {
    private:
        VkBuffer m_Buffer = nullptr;
        VkDevice m_Device = nullptr;
        VkDeviceSize m_Size = 0;
        VkMemoryRequirements m_MemoryInfo{};

        void Move(Buffer &other) noexcept {
           m_Buffer = other.m_Buffer;
           m_Device = other.m_Device;
           m_Size = other.m_Size;
           m_MemoryInfo = other.m_MemoryInfo;
           other.m_Buffer = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Buffer) vkDestroyBuffer(m_Device, m_Buffer, nullptr); }

    public:
        ~Buffer() { Release(); }

        Buffer() = default;

        Buffer(VkDevice device, const std::set<uint32_t> &queueIndices, VkDeviceSize size, VkBufferUsageFlags usage,
               VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE) : m_Device(device), m_Size(size) {

           VkBufferCreateInfo bufferInfo = {};
           bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
           bufferInfo.size = m_Size;
           bufferInfo.usage = usage;
           bufferInfo.sharingMode = sharingMode;

           if (queueIndices.size() < 2) {
              bufferInfo.queueFamilyIndexCount = 0;
              bufferInfo.pQueueFamilyIndices = nullptr;
           } else {
              std::vector<uint32_t> uniqueIndices(queueIndices.begin(), queueIndices.end());
              bufferInfo.queueFamilyIndexCount = uniqueIndices.size();
              bufferInfo.pQueueFamilyIndices = uniqueIndices.data();
           }

           if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS)
              throw std::runtime_error("failed to create buffer!");

           vkGetBufferMemoryRequirements(m_Device, m_Buffer, &m_MemoryInfo);
        }

        Buffer(const Buffer &other) = delete;

        auto operator=(const Buffer &other) -> Buffer & = delete;

        Buffer(Buffer &&other) noexcept { Move(other); }

        auto operator=(Buffer &&other) noexcept -> Buffer & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto size() const -> VkDeviceSize { return m_Size; }

        auto ptr() const noexcept -> const VkBuffer * { return &m_Buffer; }

        auto data() const noexcept -> const VkBuffer & { return m_Buffer; }

        auto MemoryInfo() const -> const VkMemoryRequirements & { return m_MemoryInfo; }

        void BindMemory(VkDeviceMemory memory, VkDeviceSize offset) {
           if (vkBindBufferMemory(m_Device, m_Buffer, memory, offset) != VK_SUCCESS)
              throw std::runtime_error("failed to bind memory to buffer!");
        }
    };


    class DeviceMemory {
    public:
        enum class UsageType {
            DEVICE_LOCAL_LARGE
        };

    private:
        VkDevice m_Device = nullptr;
        VkDeviceMemory m_Memory = nullptr;

        void Move(DeviceMemory &other) noexcept {
           m_Memory = other.m_Memory;
           m_Device = other.m_Device;
           other.m_Memory = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept {
           if (m_Memory) {
              vkFreeMemory(m_Device, m_Memory, nullptr);
           }
        }

        void Allocate(uint32_t memoryTypeIdx, VkDeviceSize size) {
           VkMemoryAllocateInfo allocInfo = {};
           allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
           allocInfo.allocationSize = size;
           allocInfo.memoryTypeIndex = memoryTypeIdx;

           if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory) != VK_SUCCESS)
              throw std::runtime_error("failed to allocate buffer memory!");
        }

    public:
        void *m_Mapped = nullptr;

        ~DeviceMemory() { Release(); }

        DeviceMemory() = default;

        DeviceMemory(VkPhysicalDevice physDevice,
                     VkDevice device,
                     const std::vector<const Image *> &images,
                     std::optional<VkDeviceSize> sizeOverride,
                     VkMemoryPropertyFlags flags);

        DeviceMemory(VkPhysicalDevice physDevice,
                     VkDevice device,
                     const std::vector<const Buffer *> &buffers,
                     VkMemoryPropertyFlags flags);

        DeviceMemory(VkDevice device, uint32_t memoryTypeIdx, VkDeviceSize size) : m_Device(device) {
           Allocate(memoryTypeIdx, size);
        }

        DeviceMemory(const DeviceMemory &other) = delete;

        auto operator=(const DeviceMemory &other) -> DeviceMemory & = delete;

        DeviceMemory(DeviceMemory &&other) noexcept { Move(other); }

        auto operator=(DeviceMemory &&other) noexcept -> DeviceMemory & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkDeviceMemory * { return &m_Memory; }

        auto data() const noexcept -> const VkDeviceMemory & { return m_Memory; }

        void MapMemory(VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags = 0) {
           m_Mapped = nullptr;
           if (vkMapMemory(m_Device, m_Memory, offset, size, flags, &m_Mapped) != VK_SUCCESS) {
              throw std::runtime_error("failed to map device memory!");
           }
        }

        void UnmapMemory() {
           if (m_Mapped) {
              vkUnmapMemory(m_Device, m_Memory);
              m_Mapped = nullptr;
           }
        }

        void Flush(VkDeviceSize offset, VkDeviceSize size) {
           VkMappedMemoryRange mappedRange = {};
           mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
           mappedRange.memory = m_Memory;
           mappedRange.offset = offset;
           mappedRange.size = size;
           if (vkFlushMappedMemoryRanges(m_Device, 1, &mappedRange) != VK_SUCCESS) {
              throw std::runtime_error("failed to flush memory!");
           }
        }
    };


    class DescriptorPool {
    private:
        VkDevice m_Device = nullptr;
        VkDescriptorPool m_Pool = nullptr;

        void Move(DescriptorPool &other) noexcept {
           m_Device = other.m_Device;
           m_Pool = other.m_Pool;
           other.m_Device = nullptr;
           other.m_Pool = nullptr;
        }

        void Release() noexcept { if (m_Pool) vkDestroyDescriptorPool(m_Device, m_Pool, nullptr); }

    public:
        ~DescriptorPool() { Release(); }

        DescriptorPool() = default;

        DescriptorPool(VkDevice device,
                       const std::vector<VkDescriptorPoolSize> &poolSizes,
                       uint32_t maxSets,
                       VkDescriptorPoolCreateFlags flags = 0) : m_Device(device) {

           VkDescriptorPoolCreateInfo poolInfo = {};
           poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
           poolInfo.flags = flags;
           poolInfo.poolSizeCount = poolSizes.size();
           poolInfo.pPoolSizes = poolSizes.data();
           poolInfo.maxSets = maxSets;

           if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_Pool) != VK_SUCCESS) {
              throw std::runtime_error("failed to create descriptor pool!");
           }
        }

        DescriptorPool(const DescriptorPool &other) = delete;

        auto operator=(const DescriptorPool &other) -> DescriptorPool & = delete;

        DescriptorPool(DescriptorPool &&other) noexcept { Move(other); }

        auto operator=(DescriptorPool &&other) noexcept -> DescriptorPool & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkDescriptorPool * { return &m_Pool; }

        auto data() const noexcept -> const VkDescriptorPool & { return m_Pool; }
    };


    class DescriptorSets {
    private:
        VkDevice m_Device = nullptr;
        VkDescriptorPool m_Pool = nullptr;
        std::vector<VkDescriptorSet> m_Sets;

        void Move(DescriptorSets &other) noexcept {
           m_Device = other.m_Device;
           m_Pool = other.m_Pool;
           m_Sets = std::move(other.m_Sets);
           other.m_Device = nullptr;
           other.m_Pool = nullptr;
           other.m_Sets.clear();
        }

    public:
        void Release() noexcept {
           m_Sets.clear();
        }

        ~DescriptorSets() { Release(); }

        DescriptorSets() = default;

        DescriptorSets(VkDevice device,
                       VkDescriptorPool pool,
                       const std::vector<VkDescriptorSetLayout> &layouts,
                       const std::vector<uint32_t>& variableCounts)
                : m_Device(device), m_Pool(pool) {

           VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocInfo = {};
           variableDescriptorCountAllocInfo.sType =
                   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
           variableDescriptorCountAllocInfo.descriptorSetCount = variableCounts.size();
           variableDescriptorCountAllocInfo.pDescriptorCounts = variableCounts.data();

           VkDescriptorSetAllocateInfo allocInfo = {};
           allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
           allocInfo.descriptorPool = pool;
           allocInfo.descriptorSetCount = layouts.size();
           allocInfo.pSetLayouts = layouts.data();
           if (!variableCounts.empty()) {
              allocInfo.pNext = &variableDescriptorCountAllocInfo;
           }

           m_Sets.resize(layouts.size());
//            VkResult result = ;
           vk::CheckResult(vkAllocateDescriptorSets(device, &allocInfo, m_Sets.data()),
                           "[vkAllocateDescriptorSets] Failed to allocate descriptor sets");
//            if (result != VK_SUCCESS) {
//               vk::errorString(result);
//                std::ostringstream msg;
//                msg << , error code: " << result;
//                throw std::runtime_error(msg.str().c_str());
//            }
        }

        DescriptorSets(const DescriptorSets &other) = delete;

        auto operator=(const DescriptorSets &other) -> DescriptorSets & = delete;

        DescriptorSets(DescriptorSets &&other) noexcept { Move(other); }

        auto operator=(DescriptorSets &&other) noexcept -> DescriptorSets & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto get(size_t index) const -> const VkDescriptorSet & {
           if (index >= m_Sets.size()) throw std::runtime_error("out of bounds descriptor set indexing");
           return m_Sets[index];
        };

        auto operator[](size_t index) const -> const VkDescriptorSet & {
           return get(index);
        };

        auto data() const -> const VkDescriptorSet * { return m_Sets.data(); }

        auto size() const -> uint32_t { return m_Sets.size(); }
    };


    class DescriptorSetLayout {
    private:
        VkDevice m_Device = nullptr;
        VkDescriptorSetLayout m_Layout = nullptr;
        VkDescriptorSetLayoutCreateFlags m_LayoutFlags = 0;

        void Move(DescriptorSetLayout &other) noexcept {
           m_Device = other.m_Device;
           m_Layout = other.m_Layout;
           other.m_Device = nullptr;
           other.m_Layout = nullptr;
        }

        void Release() noexcept { if (m_Layout) vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr); }

    public:
        ~DescriptorSetLayout() { Release(); }

        DescriptorSetLayout() = default;

        DescriptorSetLayout(VkDevice device,
                            const std::vector<std::pair<VkDescriptorSetLayoutBinding,
                                    VkDescriptorBindingFlagsEXT>> &bindings);

        DescriptorSetLayout(const DescriptorSetLayout &other) = delete;

        auto operator=(const DescriptorSetLayout &other) -> DescriptorSetLayout & = delete;

        DescriptorSetLayout(DescriptorSetLayout &&other) noexcept { Move(other); }

        auto operator=(DescriptorSetLayout &&other) noexcept -> DescriptorSetLayout & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto layoutFlags() const noexcept -> VkDescriptorSetLayoutCreateFlags { return m_LayoutFlags; }

        auto ptr() const noexcept -> const VkDescriptorSetLayout * { return &m_Layout; }

        auto data() const noexcept -> const VkDescriptorSetLayout & { return m_Layout; }
    };


    class PipelineLayout {
    private:
        VkDevice m_Device = nullptr;
        VkPipelineLayout m_Layout = nullptr;

        void Move(PipelineLayout &other) noexcept {
           m_Device = other.m_Device;
           m_Layout = other.m_Layout;
           other.m_Device = nullptr;
           other.m_Layout = nullptr;
        }

        void Release() noexcept { if (m_Layout) vkDestroyPipelineLayout(m_Device, m_Layout, nullptr); }

    public:

        ~PipelineLayout() { Release(); }

        PipelineLayout() = default;

        explicit PipelineLayout(VkDevice device, const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts = {},
                                const std::vector<VkPushConstantRange> &pushConstants = {}) : m_Device(device) {
           VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
           pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
           pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
           pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
           pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
           pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

           if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
              throw std::runtime_error("failed to create pipeline layout!");
        }

        PipelineLayout(const PipelineLayout &other) = delete;

        auto operator=(const PipelineLayout &other) -> PipelineLayout & = delete;

        PipelineLayout(PipelineLayout &&other) noexcept { Move(other); }

        auto operator=(PipelineLayout &&other) noexcept -> PipelineLayout & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkPipelineLayout * { return &m_Layout; }

        auto data() const noexcept -> const VkPipelineLayout & { return m_Layout; }
    };


    class PipelineCache {
    private:
        VkDevice m_Device = nullptr;
        VkPipelineCache m_Cache = nullptr;

        void Move(PipelineCache &other) noexcept {
           m_Device = other.m_Device;
           m_Cache = other.m_Cache;
           other.m_Device = nullptr;
           other.m_Cache = nullptr;
        }

        void Release() noexcept { if (m_Cache) vkDestroyPipelineCache(m_Device, m_Cache, nullptr); }

    public:

        ~PipelineCache() { Release(); }

        PipelineCache() = default;

        PipelineCache(VkDevice device, size_t dataSize, const void *data) : m_Device(device) {
           VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
           pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
           pipelineCacheCreateInfo.initialDataSize = dataSize;
           pipelineCacheCreateInfo.pInitialData = data;

           if (vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo, nullptr, &m_Cache) != VK_SUCCESS)
              throw std::runtime_error("failed to create pipeline layout!");
        }

        PipelineCache(const PipelineCache &other) = delete;

        auto operator=(const PipelineCache &other) -> PipelineCache & = delete;

        PipelineCache(PipelineCache &&other) noexcept { Move(other); }

        auto operator=(PipelineCache &&other) noexcept -> PipelineCache & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkPipelineCache * { return &m_Cache; }

        auto data() const noexcept -> const VkPipelineCache & { return m_Cache; }
    };


    class Pipeline {
    private:
        VkDevice m_Device = nullptr;
        VkPipeline m_Pipeline = nullptr;

        void Move(Pipeline &other) noexcept {
           m_Device = other.m_Device;
           m_Pipeline = other.m_Pipeline;
           other.m_Device = nullptr;
           other.m_Pipeline = nullptr;
        }

        void Release() noexcept { if (m_Pipeline) vkDestroyPipeline(m_Device, m_Pipeline, nullptr); }

    public:

        ~Pipeline() { Release(); }

        Pipeline() = default;

        Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo &createInfo, VkPipelineCache cache = nullptr)
                : m_Device(device) {
           if (vkCreateGraphicsPipelines(m_Device, cache, 1, &createInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
              throw std::runtime_error("failed to create graphics pipeline!");
        }

        Pipeline(const Pipeline &other) = delete;

        auto operator=(const Pipeline &other) -> Pipeline & = delete;

        Pipeline(Pipeline &&other) noexcept { Move(other); }

        auto operator=(Pipeline &&other) noexcept -> Pipeline & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkPipeline * { return &m_Pipeline; }

        auto data() const noexcept -> const VkPipeline & { return m_Pipeline; }
    };


    class Sampler {
    private:
        VkSampler m_Sampler = nullptr;
        VkDevice m_Device = nullptr;

        void Move(Sampler &other) noexcept {
           m_Sampler = other.m_Sampler;
           m_Device = other.m_Device;
           other.m_Sampler = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Sampler) vkDestroySampler(m_Device, m_Sampler, nullptr); }

    public:
        ~Sampler() { Release(); }

        Sampler() = default;

        Sampler(VkDevice device, float maxLod);

        Sampler(const Sampler &other) = delete;

        auto operator=(const Sampler &other) -> Sampler & = delete;

        Sampler(Sampler &&other) noexcept { Move(other); }

        auto operator=(Sampler &&other) noexcept -> Sampler & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkSampler * { return &m_Sampler; }

        auto data() const noexcept -> const VkSampler & { return m_Sampler; }
    };


    class ShaderModule {
    public:
        struct VertexAttribute {
            std::string name;
            VkFormat format;
            uint32_t size;

            auto operator==(const VertexAttribute &other) const -> bool {
               return format == other.format && size == other.size;
            }

            auto operator!=(const VertexAttribute &other) const -> bool {
               return !operator==(other);
            }
        };

        struct VertexBinding {
            std::vector<VertexAttribute> vertexLayout;
            uint32_t binding;
            VkVertexInputRate inputRate;
        };


    private:
        std::unordered_map<uint32_t, UniformBinding> m_UniformBindings;
        std::unordered_map<uint32_t, SamplerBinding> m_SamplerBindings;
        std::unordered_map<uint32_t, VkPushConstantRange> m_PushRanges;
        std::vector<VertexBinding> m_VertexInputBindings;
        std::vector<VertexBinding> m_VertexOutputBindings;
        std::string m_Filepath;
        std::string m_Name;

        VkDevice m_Device = nullptr;
        VkShaderModule m_Module = nullptr;
        size_t m_CodeSize = 0;

        void Move(ShaderModule &other) noexcept {
           m_Device = other.m_Device;
           m_Module = other.m_Module;
           m_CodeSize = other.m_CodeSize;
           other.m_Module = nullptr;
        }

        void Release() noexcept { if (m_Module) vkDestroyShaderModule(m_Device, m_Module, nullptr); }

        static auto ParseSpirVType(const spirv_cross::SPIRType &type) -> std::pair<VkFormat, uint32_t>;

        void ExtractIO(const spirv_cross::CompilerGLSL &compiler,
                       const spirv_cross::SmallVector<spirv_cross::Resource> &resources,
                       std::vector<VertexBinding> &output);

    public:
        ShaderModule() = default;

        ShaderModule(VkDevice device, const std::string &filename);

        ~ShaderModule() { Release(); }

        ShaderModule(const ShaderModule &other) = delete;

        auto operator=(const ShaderModule &other) -> ShaderModule & = delete;

        ShaderModule(ShaderModule &&other) noexcept { Move(other); }

        auto operator=(ShaderModule &&other) noexcept -> ShaderModule & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto GetPushRanges() const -> const auto & { return m_PushRanges; }

        auto GetUniformBindings() const -> const auto & { return m_UniformBindings; }

        auto GetSamplerBindings() const -> const auto & { return m_SamplerBindings; }

        auto GetInputBindings() const -> const auto & { return m_VertexInputBindings; }

        auto GetOutputBindings() const -> const auto & { return m_VertexOutputBindings; }

        auto ptr() const noexcept -> const VkShaderModule * { return &m_Module; }

        auto data() const noexcept -> const VkShaderModule & { return m_Module; }

        auto size() const -> size_t { return m_CodeSize; }
    };


    class Surface {
    private:
        VkInstance m_Instance = nullptr;
        VkSurfaceKHR m_Surface = nullptr;

        void Move(Surface &other) noexcept {
           m_Surface = other.m_Surface;
           m_Instance = other.m_Instance;
           other.m_Surface = nullptr;
           other.m_Instance = nullptr;
        }

        void Release() noexcept { if (m_Surface) vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr); }

    public:
        ~Surface() { Release(); }

        Surface() = default;

        Surface(VkInstance instance, GLFWwindow *window);

        Surface(const Surface &other) = delete;

        auto operator=(const Surface &other) -> Surface & = delete;

        Surface(Surface &&other) noexcept { Move(other); }

        auto operator=(Surface &&other) noexcept -> Surface & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        auto ptr() const noexcept -> const VkSurfaceKHR * { return &m_Surface; }

        auto data() const noexcept -> const VkSurfaceKHR & { return m_Surface; }
    };


    class Instance {
    private:
        VkInstance m_Instance = nullptr;
        VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;

        static constexpr std::array<const char *, 0> s_RequiredLayers = {};

        static constexpr std::array<const char *, 1> s_ValidationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };

        static constexpr std::array<const char *, 1> s_OptionalLayers = {
                "VK_LAYER_LUNARG_monitor"
        };

        void Move(Instance &other) noexcept {
           m_Instance = other.m_Instance;
           m_DebugMessenger = other.m_DebugMessenger;
           other.m_Instance = nullptr;
           other.m_DebugMessenger = nullptr;
        }

        void Release() noexcept;

    public:
        ~Instance() { Release(); }

        explicit Instance(bool enableValidation);

        Instance(const Instance &other) = delete;

        auto operator=(const Instance &other) -> Instance & = delete;

        Instance(Instance &&other) noexcept { Move(other); }

        auto operator=(Instance &&other) noexcept -> Instance & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        void setupDebugMessenger();

        auto ptr() const noexcept -> const VkInstance * { return &m_Instance; }

        auto data() const noexcept -> const VkInstance & { return m_Instance; }

    };


    class Swapchain {
    private:
        VkDevice m_Device = nullptr;
        VkSwapchainKHR m_Swapchain = nullptr;
        VkSurfaceCapabilitiesKHR m_Capabilities{};
        VkFormat m_Format = VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent = {};
        std::vector<VkImage> m_Images;
        std::vector<ImageView> m_ImageViews;

        void Move(Swapchain &other) noexcept {
           m_Swapchain = other.m_Swapchain;
           m_Device = other.m_Device;
           m_Format = other.m_Format;
           m_Extent = other.m_Extent;
           m_Capabilities = other.m_Capabilities;
           m_Images = std::move(other.m_Images);
           m_ImageViews = std::move(other.m_ImageViews);
           other.m_Swapchain = nullptr;
           other.m_Device = nullptr;
        }

    public:
        ~Swapchain() { Release(); }

        Swapchain() = default;

        Swapchain(VkDevice device, const std::set<uint32_t> &queueIndices, VkExtent2D extent, VkSurfaceKHR surface,
                  const VkSurfaceCapabilitiesKHR &capabilities, const VkSurfaceFormatKHR &surfaceFormat,
                  VkPresentModeKHR presentMode);

        Swapchain(const Swapchain &other) = delete;

        auto operator=(const Swapchain &other) -> Swapchain & = delete;

        Swapchain(Swapchain &&other) noexcept { Move(other); }

        auto operator=(Swapchain &&other) noexcept -> Swapchain & {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        void Release() noexcept {
           if (m_Swapchain) {
              vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
              m_Swapchain = nullptr;
           }
        }

        auto ImageFormat() const -> const VkFormat & { return m_Format; }

        auto Extent() const -> const VkExtent2D & { return m_Extent; }

        auto Images() const -> const std::vector<VkImage> & { return m_Images; }

        auto ImageCount() const -> uint32_t { return m_Images.size(); }

        auto ImageViews() const -> const std::vector<ImageView> & { return m_ImageViews; }

        auto Capabilities() const -> const VkSurfaceCapabilitiesKHR & { return m_Capabilities; }

        auto ptr() const noexcept -> const VkSwapchainKHR * { return &m_Swapchain; }

        auto data() const noexcept -> const VkSwapchainKHR & { return m_Swapchain; }
    };
}


#endif //VULKAN_VULKAN_WRAPPERS_H
