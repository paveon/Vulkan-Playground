#ifndef VULKAN_VULKAN_WRAPPERS_H
#define VULKAN_VULKAN_WRAPPERS_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <set>
#include <vector>
#include <array>
#include <mathlib.h>

typedef struct GLFWwindow GLFWwindow;

template<class T>
inline void hash_combine(std::size_t& s, const T& v) {
   std::hash<T> h;
   s ^= h(v) + 0x9e3779b9 + (s << 6u) + (s >> 2u);
}


using VertexIndex = uint32_t;

struct Vertex {
    math::vec3 pos;
    math::vec3 color;
    math::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
       VkVertexInputBindingDescription bindingDescription = {};
       bindingDescription.binding = 0;
       bindingDescription.stride = sizeof(Vertex);
       bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

       return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
       std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
       attributeDescriptions[0].binding = 0;
       attributeDescriptions[0].location = 0;
       attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
       attributeDescriptions[0].offset = 0;

       attributeDescriptions[1].binding = 0;
       attributeDescriptions[1].location = 1;
       attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
       attributeDescriptions[1].offset = offsetof(Vertex, color);

       attributeDescriptions[2].binding = 0;
       attributeDescriptions[2].location = 2;
       attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
       attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

       return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const { return pos == other.pos && color == other.color && texCoord == other.texCoord; }
};


namespace vk {
    class Semaphore {
    private:
        VkSemaphore m_Semaphore = nullptr;
        VkDevice m_Device = nullptr;

        void Move(Semaphore& other) noexcept {
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

        Semaphore(const Semaphore& other) = delete;

        Semaphore& operator=(const Semaphore& other) = delete;

        Semaphore(Semaphore&& other) noexcept { Move(other); }

        Semaphore& operator=(Semaphore&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkSemaphore* ptr() const noexcept { return &m_Semaphore; }

        const VkSemaphore& data() const noexcept { return m_Semaphore; }
    };


    class Fence {
    private:
        VkFence m_Fence = nullptr;
        VkDevice m_Device = nullptr;

        void Move(Fence& other) noexcept {
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

        Fence(const Fence& other) = delete;

        Fence& operator=(const Fence& other) = delete;

        Fence(Fence&& other) noexcept { Move(other); }

        Fence& operator=(Fence&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkFence* ptr() const noexcept { return &m_Fence; }

        const VkFence& data() const noexcept { return m_Fence; }
    };


    class CommandPool {
    private:
        VkCommandPool m_Pool = nullptr;
        VkDevice m_Device = nullptr;

        void Move(CommandPool& other) noexcept {
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

        CommandPool(const CommandPool& other) = delete;

        CommandPool& operator=(const CommandPool& other) = delete;

        CommandPool(CommandPool&& other) noexcept { Move(other); }

        CommandPool& operator=(CommandPool&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkCommandPool* ptr() const noexcept { return &m_Pool; }

        const VkCommandPool& data() const noexcept { return m_Pool; }
    };


    class CommandBuffer {
    private:
        VkCommandPool m_Pool = nullptr;
        VkCommandBuffer m_Buffer = nullptr;
        VkDevice m_Device = nullptr;

        void Move(CommandBuffer& other) noexcept {
           m_Pool = other.m_Pool;
           m_Buffer = other.m_Buffer;
           m_Device = other.m_Device;
           other.m_Pool = nullptr;
           other.m_Buffer = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Buffer) vkFreeCommandBuffers(m_Device, m_Pool, 1, &m_Buffer); }

    public:
        ~CommandBuffer() { Release(); }

        CommandBuffer() = default;

        CommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer buffer) : m_Pool(pool), m_Buffer(buffer), m_Device(device) {};

        CommandBuffer(VkDevice device, VkCommandPool pool) : m_Pool(pool), m_Device(device) {
           VkCommandBufferAllocateInfo allocInfo = {};
           allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
           allocInfo.commandPool = m_Pool;
           allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
           allocInfo.commandBufferCount = 1;

           if (vkAllocateCommandBuffers(m_Device, &allocInfo, &m_Buffer) != VK_SUCCESS)
              throw std::runtime_error("failed to allocate command buffers!");
        }

        CommandBuffer(const CommandBuffer& other) = delete;

        CommandBuffer& operator=(const CommandBuffer& other) = delete;

        CommandBuffer(CommandBuffer&& other) noexcept { Move(other); }

        CommandBuffer& operator=(CommandBuffer&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkCommandBuffer* ptr() const noexcept { return &m_Buffer; }

        const VkCommandBuffer& data() const noexcept { return m_Buffer; }

        void Begin(VkCommandBufferUsageFlags flags = 0) {
           VkCommandBufferBeginInfo beginInfo = {};
           beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
           beginInfo.flags = flags;

           if (vkBeginCommandBuffer(m_Buffer, &beginInfo) != VK_SUCCESS)
              throw std::runtime_error("failed to begin command buffer recording!");
        }

        void End() {
           if (vkEndCommandBuffer(m_Buffer) != VK_SUCCESS)
              throw std::runtime_error("failed to end command buffer recording!");
        }

        void Submit(VkQueue cmdQueue) {
           VkSubmitInfo submitInfo = {};
           submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
           Submit(submitInfo, cmdQueue);
        }

        void Submit(VkSubmitInfo& submitInfo, VkQueue cmdQueue, VkFence fence = nullptr) {
           submitInfo.commandBufferCount = 1;
           submitInfo.pCommandBuffers = &m_Buffer;
           vkQueueSubmit(cmdQueue, 1, &submitInfo, fence);
        }
    };


    class Framebuffer {
    private:
        VkFramebuffer m_Framebuffer = nullptr;
        VkDevice m_Device = nullptr;

        void Move(Framebuffer& other) noexcept {
           m_Framebuffer = other.m_Framebuffer;
           m_Device = other.m_Device;
           other.m_Framebuffer = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Framebuffer) vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr); }

    public:
        ~Framebuffer() { Release(); }

        Framebuffer() = default;

        Framebuffer(VkDevice device, VkRenderPass renderPass, const VkExtent2D& extent, const std::vector<VkImageView>& attachments) : m_Device(device) {
           VkFramebufferCreateInfo framebufferInfo = {};
           framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
           framebufferInfo.renderPass = renderPass;
           framebufferInfo.width = extent.width;
           framebufferInfo.height = extent.height;
           framebufferInfo.layers = 1;
           framebufferInfo.attachmentCount = attachments.size();
           framebufferInfo.pAttachments = attachments.data();

           if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS)
              throw std::runtime_error("failed to create framebuffer!");
        }

        Framebuffer(const Framebuffer& other) = delete;

        Framebuffer& operator=(const Framebuffer& other) = delete;

        Framebuffer(Framebuffer&& other) noexcept { Move(other); }

        Framebuffer& operator=(Framebuffer&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkFramebuffer* ptr() const noexcept { return &m_Framebuffer; }

        const VkFramebuffer& data() const noexcept { return m_Framebuffer; }
    };

    class Image;

    class ImageView {
    private:
        VkImageView m_ImageView = nullptr;
        VkDevice m_Device = nullptr;

        void Move(ImageView& other) noexcept {
           m_ImageView = other.m_ImageView;
           m_Device = other.m_Device;
           other.m_ImageView = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_ImageView) vkDestroyImageView(m_Device, m_ImageView, nullptr); }

    public:
        ~ImageView() { Release(); }

        ImageView() = default;

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

        ImageView(VkDevice, const Image& image, VkImageAspectFlags aspectFlags);

        ImageView(const ImageView& other) = delete;

        ImageView& operator=(const ImageView& other) = delete;

        ImageView(ImageView&& other) noexcept { Move(other); }

        ImageView& operator=(ImageView&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkImageView* ptr() const noexcept { return &m_ImageView; }

        const VkImageView& data() const noexcept { return m_ImageView; }
    };


    class Image {
    private:
        friend class ImageView;

        VkImage m_Image = nullptr;
        VkDevice m_Device = nullptr;
        VkFormat m_Format = VK_FORMAT_UNDEFINED;
        VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkPipelineStageFlags m_CurrentStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        uint32_t m_MipLevels = 1;

        static constexpr VkImageMemoryBarrier s_BaseBarrier = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                nullptr,
                0,
                0,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                nullptr,
                {
                        VK_IMAGE_ASPECT_COLOR_BIT,
                        0,
                        1,
                        0,
                        1
                }
        };

        void Move(Image& other) noexcept {
           m_Image = other.m_Image;
           m_Device = other.m_Device;
           m_Format = other.m_Format;
           m_CurrentLayout = other.m_CurrentLayout;
           m_CurrentStage = other.m_CurrentStage;
           m_MipLevels = other.m_MipLevels;
           Extent = other.Extent;
           other.m_Image = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Image) vkDestroyImage(m_Device, m_Image, nullptr); }

    public:
        VkExtent2D Extent = {};

        ~Image() { Release(); }

        Image() = default;

        Image(VkDevice device, const std::set<uint32_t>& queueIndices, const VkExtent2D& extent, uint32_t mipLevels, VkSampleCountFlagBits samples,
              VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage) : m_Device(device), m_Format(format), m_MipLevels(mipLevels),
                                                                                Extent(extent) {

           VkImageCreateInfo imageInfo = {};
           imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
           imageInfo.imageType = VK_IMAGE_TYPE_2D;
           imageInfo.extent.width = extent.width;
           imageInfo.extent.height = extent.height;
           imageInfo.extent.depth = 1;
           imageInfo.mipLevels = m_MipLevels;
           imageInfo.arrayLayers = 1;
           imageInfo.format = m_Format;
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

           if (vkCreateImage(m_Device, &imageInfo, nullptr, &m_Image) != VK_SUCCESS)
              throw std::runtime_error("failed to create image!");
        }

        Image(VkDevice device, const VkImageCreateInfo& createInfo) : m_Device(device) {
           m_Format = createInfo.format;
           m_MipLevels = createInfo.mipLevels;
           Extent.width = createInfo.extent.width;
           Extent.height = createInfo.extent.height;
           if (vkCreateImage(m_Device, &createInfo, nullptr, &m_Image) != VK_SUCCESS)
              throw std::runtime_error("failed to create image!");
        }

        Image(const Image& other) = delete;

        Image& operator=(const Image& other) = delete;

        Image(Image&& other) noexcept { Move(other); }

        Image& operator=(Image&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkImage* ptr() const noexcept { return &m_Image; }

        const VkImage& data() const noexcept { return m_Image; }

        ImageView createView(VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) {
           return ImageView(m_Device, m_Image, format, m_MipLevels, aspectFlags);
        }

        uint32_t MipLevels() { return m_MipLevels; }

        void ChangeLayout(const CommandBuffer& cmdBuffer, VkImageLayout newLayout);

        void ChangeLayout(const CommandBuffer& cmdBuffer, VkImageLayout newLayout, VkPipelineStageFlags dstStage,
                          VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

        void GenerateMipmaps(VkPhysicalDevice physicalDevice, const CommandBuffer& cmdBuffer);
    };


    inline ImageView::ImageView(VkDevice device, const Image& image, VkImageAspectFlags aspectFlags) : m_Device(device) {
       VkImageViewCreateInfo createInfo = {};
       createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
       createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
       createInfo.format = image.m_Format;
       createInfo.image = image.data();
       createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
       createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
       createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
       createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
       createInfo.subresourceRange.aspectMask = aspectFlags;
       createInfo.subresourceRange.baseMipLevel = 0;
       createInfo.subresourceRange.levelCount = image.m_MipLevels;
       createInfo.subresourceRange.baseArrayLayer = 0;
       createInfo.subresourceRange.layerCount = 1;

       if (vkCreateImageView(m_Device, &createInfo, nullptr, &m_ImageView) != VK_SUCCESS)
          throw std::runtime_error("failed to create image views!");
    }


    class Buffer {
    private:
        VkBuffer m_Buffer = nullptr;
        VkDevice m_Device = nullptr;

        void Move(Buffer& other) noexcept {
           m_Buffer = other.m_Buffer;
           m_Device = other.m_Device;
           other.m_Buffer = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Buffer) vkDestroyBuffer(m_Device, m_Buffer, nullptr); }

    public:
        ~Buffer() { Release(); }

        Buffer() = default;

        Buffer(VkDevice device, const std::set<uint32_t>& queueIndices, VkDeviceSize bufferSize, VkBufferUsageFlags usage,
               VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE) : m_Device(device) {

           VkBufferCreateInfo bufferInfo = {};
           bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
           bufferInfo.size = bufferSize;
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
        }

        Buffer(const Buffer& other) = delete;

        Buffer& operator=(const Buffer& other) = delete;

        Buffer(Buffer&& other) noexcept { Move(other); }

        Buffer& operator=(Buffer&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkBuffer* ptr() const noexcept { return &m_Buffer; }

        const VkBuffer& data() const noexcept { return m_Buffer; }
    };


    class DeviceMemory {
    private:
        VkDevice m_Device = nullptr;
        VkDeviceMemory m_Memory = nullptr;

        void Move(DeviceMemory& other) noexcept {
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

    public:
        ~DeviceMemory() { Release(); }

        DeviceMemory() = default;

        DeviceMemory(VkDevice device, uint32_t memoryTypeIdx, VkDeviceSize size) : m_Device(device) {
           VkMemoryAllocateInfo allocInfo = {};
           allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
           allocInfo.allocationSize = size;
           allocInfo.memoryTypeIndex = memoryTypeIdx;

           if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory) != VK_SUCCESS)
              throw std::runtime_error("failed to allocate buffer memory!");
        }

        DeviceMemory(const DeviceMemory& other) = delete;

        DeviceMemory& operator=(const DeviceMemory& other) = delete;

        DeviceMemory(DeviceMemory&& other) noexcept { Move(other); }

        DeviceMemory& operator=(DeviceMemory&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkDeviceMemory* ptr() const noexcept { return &m_Memory; }

        const VkDeviceMemory& data() const noexcept { return m_Memory; }
    };


    class DescriptorPool {
    private:
        VkDevice m_Device = nullptr;
        VkDescriptorPool m_Pool = nullptr;

        void Move(DescriptorPool& other) noexcept {
           m_Device = other.m_Device;
           m_Pool = other.m_Pool;
           other.m_Device = nullptr;
           other.m_Pool = nullptr;
        }

        void Release() noexcept { if (m_Pool) vkDestroyDescriptorPool(m_Device, m_Pool, nullptr); }

    public:
        ~DescriptorPool() { Release(); }

        DescriptorPool() = default;

        DescriptorPool(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets) : m_Device(device) {
           VkDescriptorPoolCreateInfo poolInfo = {};
           poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
           poolInfo.poolSizeCount = poolSizes.size();
           poolInfo.pPoolSizes = poolSizes.data();
           poolInfo.maxSets = maxSets;

           if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_Pool) != VK_SUCCESS) {
              throw std::runtime_error("failed to create descriptor pool!");
           }
        }

        DescriptorPool(const DescriptorPool& other) = delete;

        DescriptorPool& operator=(const DescriptorPool& other) = delete;

        DescriptorPool(DescriptorPool&& other) noexcept { Move(other); }

        DescriptorPool& operator=(DescriptorPool&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkDescriptorPool* ptr() const noexcept { return &m_Pool; }

        const VkDescriptorPool& data() const noexcept { return m_Pool; }
    };


    class CommandBuffers {
    private:
        VkDevice m_Device = nullptr;
        VkCommandPool m_Pool = nullptr;
        std::vector<VkCommandBuffer> m_Buffers;

        void Move(CommandBuffers& other) noexcept {
           m_Device = other.m_Device;
           m_Pool = other.m_Pool;
           m_Buffers = std::move(other.m_Buffers);
           other.m_Device = nullptr;
           other.m_Pool = nullptr;
           other.m_Buffers.clear();
        }

        void Release() noexcept { if (!m_Buffers.empty()) vkFreeCommandBuffers(m_Device, m_Pool, m_Buffers.size(), m_Buffers.data()); }

    public:
        ~CommandBuffers() { Release(); }

        CommandBuffers() = default;

        CommandBuffers(VkDevice device, VkCommandPool pool, uint32_t count) : m_Device(device), m_Pool(pool) {
           VkCommandBufferAllocateInfo allocInfo = {};
           allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
           allocInfo.commandPool = m_Pool;
           allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
           allocInfo.commandBufferCount = count;

           m_Buffers.resize(count);
           if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_Buffers.data()) != VK_SUCCESS)
              throw std::runtime_error("failed to allocate command buffers!");
        }

        CommandBuffers(const CommandBuffers& other) = delete;

        CommandBuffers& operator=(const CommandBuffers& other) = delete;

        CommandBuffers(CommandBuffers&& other) noexcept { Move(other); }

        CommandBuffers& operator=(CommandBuffers&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkCommandBuffer& operator[](size_t index) const {
           if (index >= m_Buffers.size()) throw std::runtime_error("out of bounds command buffer indexing");
           return m_Buffers[index];
        };

        const VkCommandBuffer* data() const { return m_Buffers.data(); }

        uint32_t size() const { return m_Buffers.size(); }
    };


    class DescriptorSets {
    private:
        VkDevice m_Device = nullptr;
        VkDescriptorPool m_Pool = nullptr;
        std::vector<VkDescriptorSet> m_Sets;

        void Move(DescriptorSets& other) noexcept {
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

        DescriptorSets(VkDevice device, VkDescriptorPool pool, const std::vector<VkDescriptorSetLayout>& layouts) : m_Device(device), m_Pool(pool) {
           VkDescriptorSetAllocateInfo allocInfo = {};
           allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
           allocInfo.descriptorPool = pool;
           allocInfo.descriptorSetCount = layouts.size();
           allocInfo.pSetLayouts = layouts.data();

           m_Sets.resize(layouts.size());
           if (vkAllocateDescriptorSets(device, &allocInfo, m_Sets.data()) != VK_SUCCESS) {
              throw std::runtime_error("failed to allocate descriptor sets!");
           }
        }

        DescriptorSets(const DescriptorSets& other) = delete;

        DescriptorSets& operator=(const DescriptorSets& other) = delete;

        DescriptorSets(DescriptorSets&& other) noexcept { Move(other); }

        DescriptorSets& operator=(DescriptorSets&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkDescriptorSet& operator[](size_t index) const {
           if (index >= m_Sets.size()) throw std::runtime_error("out of bounds command buffer indexing");
           return m_Sets[index];
        };

        const VkDescriptorSet* data() const { return m_Sets.data(); }

        uint32_t size() const { return m_Sets.size(); }
    };


    class DescriptorSetLayout {
    private:
        VkDevice m_Device = nullptr;
        VkDescriptorSetLayout m_Layout = nullptr;

        void Move(DescriptorSetLayout& other) noexcept {
           m_Device = other.m_Device;
           m_Layout = other.m_Layout;
           other.m_Device = nullptr;
           other.m_Layout = nullptr;
        }

        void Release() noexcept { if (m_Layout) vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr); }

    public:
        ~DescriptorSetLayout() { Release(); }

        DescriptorSetLayout() = default;

        DescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings) : m_Device(device) {
           VkDescriptorSetLayoutCreateInfo layoutInfo = {};
           layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
           layoutInfo.bindingCount = bindings.size();
           layoutInfo.pBindings = bindings.data();

           if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
              throw std::runtime_error("failed to create descriptor set layout!");
        }

        DescriptorSetLayout(const DescriptorSetLayout& other) = delete;

        DescriptorSetLayout& operator=(const DescriptorSetLayout& other) = delete;

        DescriptorSetLayout(DescriptorSetLayout&& other) noexcept { Move(other); }

        DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkDescriptorSetLayout* ptr() const noexcept { return &m_Layout; }

        const VkDescriptorSetLayout& data() const noexcept { return m_Layout; }
    };


    class PipelineLayout {
    private:
        VkDevice m_Device = nullptr;
        VkPipelineLayout m_Layout = nullptr;

        void Move(PipelineLayout& other) noexcept {
           m_Device = other.m_Device;
           m_Layout = other.m_Layout;
           other.m_Device = nullptr;
           other.m_Layout = nullptr;
        }

        void Release() noexcept { if (m_Layout) vkDestroyPipelineLayout(m_Device, m_Layout, nullptr); }

    public:

        ~PipelineLayout() { Release(); }

        PipelineLayout() = default;

        explicit PipelineLayout(VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts = {},
                                const std::vector<VkPushConstantRange>& pushConstants = {}) : m_Device(device) {
           VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
           pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
           pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
           pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
           pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
           pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

           if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
              throw std::runtime_error("failed to create pipeline layout!");
        }

        PipelineLayout(const PipelineLayout& other) = delete;

        PipelineLayout& operator=(const PipelineLayout& other) = delete;

        PipelineLayout(PipelineLayout&& other) noexcept { Move(other); }

        PipelineLayout& operator=(PipelineLayout&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkPipelineLayout* ptr() const noexcept { return &m_Layout; }

        const VkPipelineLayout& data() const noexcept { return m_Layout; }
    };


    class PipelineCache {
    private:
        VkDevice m_Device = nullptr;
        VkPipelineCache m_Cache = nullptr;

        void Move(PipelineCache& other) noexcept {
           m_Device = other.m_Device;
           m_Cache = other.m_Cache;
           other.m_Device = nullptr;
           other.m_Cache = nullptr;
        }

        void Release() noexcept { if (m_Cache) vkDestroyPipelineCache(m_Device, m_Cache, nullptr); }

    public:

        ~PipelineCache() { Release(); }

        PipelineCache() = default;

        PipelineCache(VkDevice device, size_t dataSize, const void* data) : m_Device(device) {
           VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
           pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
           pipelineCacheCreateInfo.initialDataSize = dataSize;
           pipelineCacheCreateInfo.pInitialData = data;

           if (vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo, nullptr, &m_Cache) != VK_SUCCESS)
              throw std::runtime_error("failed to create pipeline layout!");
        }

        PipelineCache(const PipelineCache& other) = delete;

        PipelineCache& operator=(const PipelineCache& other) = delete;

        PipelineCache(PipelineCache&& other) noexcept { Move(other); }

        PipelineCache& operator=(PipelineCache&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkPipelineCache* ptr() const noexcept { return &m_Cache; }

        const VkPipelineCache& data() const noexcept { return m_Cache; }
    };


    class Pipeline {
    private:
        VkDevice m_Device = nullptr;
        VkPipeline m_Pipeline = nullptr;

        void Move(Pipeline& other) noexcept {
           m_Device = other.m_Device;
           m_Pipeline = other.m_Pipeline;
           other.m_Device = nullptr;
           other.m_Pipeline = nullptr;
        }

        void Release() noexcept { if (m_Pipeline) vkDestroyPipeline(m_Device, m_Pipeline, nullptr); }

    public:

        ~Pipeline() { Release(); }

        Pipeline() = default;

        Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& createInfo, VkPipelineCache cache = nullptr) : m_Device(device) {
           if (vkCreateGraphicsPipelines(m_Device, cache, 1, &createInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
              throw std::runtime_error("failed to create graphics pipeline!");
        }

        Pipeline(const Pipeline& other) = delete;

        Pipeline& operator=(const Pipeline& other) = delete;

        Pipeline(Pipeline&& other) noexcept { Move(other); }

        Pipeline& operator=(Pipeline&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkPipeline* ptr() const noexcept { return &m_Pipeline; }

        const VkPipeline& data() const noexcept { return m_Pipeline; }
    };


    class Sampler {
    private:
        VkSampler m_Sampler = nullptr;
        VkDevice m_Device = nullptr;

        void Move(Sampler& other) noexcept {
           m_Sampler = other.m_Sampler;
           m_Device = other.m_Device;
           other.m_Sampler = nullptr;
           other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Sampler) vkDestroySampler(m_Device, m_Sampler, nullptr); }

    public:
        ~Sampler() { Release(); }

        Sampler() = default;

        explicit Sampler(VkDevice device, float maxLod) : m_Device(device) {
           VkSamplerCreateInfo samplerInfo = {};
           samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
           samplerInfo.magFilter = VK_FILTER_LINEAR;
           samplerInfo.minFilter = VK_FILTER_LINEAR;
           samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
           samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
           samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
           samplerInfo.anisotropyEnable = VK_TRUE;
           samplerInfo.maxAnisotropy = 16;
           samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
           samplerInfo.unnormalizedCoordinates = VK_FALSE;
           samplerInfo.compareEnable = VK_FALSE;
           samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
           samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
           samplerInfo.mipLodBias = 0.0f;
           samplerInfo.minLod = 0.0f;
           samplerInfo.maxLod = maxLod / 2;

           if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
              throw std::runtime_error("failed to create texture sampler!");
        }

        Sampler(const Sampler& other) = delete;

        Sampler& operator=(const Sampler& other) = delete;

        Sampler(Sampler&& other) noexcept { Move(other); }

        Sampler& operator=(Sampler&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkSampler* ptr() const noexcept { return &m_Sampler; }

        const VkSampler& data() const noexcept { return m_Sampler; }
    };


    class ShaderModule {
    private:
        VkDevice m_Device = nullptr;
        VkShaderModule m_Module = nullptr;
        size_t m_CodeSize;

    public:
        ShaderModule(VkDevice device, const std::string& filename);

        ~ShaderModule() {
           if (m_Module) vkDestroyShaderModule(m_Device, m_Module, nullptr);
        }

        VkPipelineShaderStageCreateInfo createInfo(VkShaderStageFlagBits stage) {
           VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
           fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
           fragShaderStageInfo.stage = stage;
           fragShaderStageInfo.module = m_Module;
           fragShaderStageInfo.pName = "main";
           return fragShaderStageInfo;
        }

        const VkShaderModule* ptr() const noexcept { return &m_Module; }

        const VkShaderModule& data() const noexcept { return m_Module; }

        size_t size() const { return m_CodeSize; }
    };


    class Surface {
    private:
        VkInstance m_Instance = nullptr;
        VkSurfaceKHR m_Surface = nullptr;

        void Move(Surface& other) noexcept {
           m_Surface = other.m_Surface;
           m_Instance = other.m_Instance;
           other.m_Surface = nullptr;
           other.m_Instance = nullptr;
        }

        void Release() noexcept { if (m_Surface) vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr); }

    public:
        ~Surface() { Release(); }

        Surface() = default;

        Surface(VkInstance instance, GLFWwindow* window);

        Surface(const Surface& other) = delete;

        Surface& operator=(const Surface& other) = delete;

        Surface(Surface&& other) noexcept { Move(other); }

        Surface& operator=(Surface&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        const VkSurfaceKHR* ptr() const noexcept { return &m_Surface; }

        const VkSurfaceKHR& data() const noexcept { return m_Surface; }
    };


    class Instance {
    private:
        VkInstance m_Instance = nullptr;
        VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;

        void Move(Instance& other) noexcept {
           m_Instance = other.m_Instance;
           m_DebugMessenger = other.m_DebugMessenger;
           other.m_Instance = nullptr;
           other.m_DebugMessenger = nullptr;
        }

        void Release() noexcept;

    public:
        ~Instance() { Release(); }

        Instance() = default;

        explicit Instance(const std::vector<const char*>& validationLayers);

        Instance(const Instance& other) = delete;

        Instance& operator=(const Instance& other) = delete;

        Instance(Instance&& other) noexcept { Move(other); }

        Instance& operator=(Instance&& other) noexcept {
           if (this == &other) return *this;
           Release();
           Move(other);
           return *this;
        };

        void setupDebugMessenger();

        const VkInstance* ptr() const noexcept { return &m_Instance; }

        const VkInstance& data() const noexcept { return m_Instance; }

    };


    class Swapchain {
    private:
        VkDevice m_Device = nullptr;
        VkSwapchainKHR m_Swapchain = nullptr;
        VkFormat m_Format = VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent = {};
        std::vector<VkImage> m_Images;
        std::vector<ImageView> m_ImageViews;

        void Move(Swapchain& other) noexcept {
           m_Swapchain = other.m_Swapchain;
           m_Device = other.m_Device;
           m_Format = other.m_Format;
           m_Extent = other.m_Extent;
           m_Images = std::move(other.m_Images);
           m_ImageViews = std::move(other.m_ImageViews);
           other.m_Swapchain = nullptr;
           other.m_Device = nullptr;
        }

    public:
        ~Swapchain() { Release(); }

        Swapchain() = default;

        Swapchain(VkDevice device, const std::set<uint32_t>& queueIndices, VkExtent2D extent, VkSurfaceKHR surface,
                  const VkSurfaceCapabilitiesKHR& capabilities, const VkSurfaceFormatKHR& surfaceFormat, VkPresentModeKHR presentMode);

        Swapchain(const Swapchain& other) = delete;

        Swapchain& operator=(const Swapchain& other) = delete;

        Swapchain(Swapchain&& other) noexcept { Move(other); }

        Swapchain& operator=(Swapchain&& other) noexcept {
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

        const VkFormat& ImageFormat() const { return m_Format; }

        const VkExtent2D& Extent() const { return m_Extent; }

        const std::vector<VkImage>& Images() const { return m_Images; }

        const std::vector<ImageView>& ImageViews() const { return m_ImageViews; }

        const VkSwapchainKHR* ptr() const noexcept { return &m_Swapchain; }

        const VkSwapchainKHR& data() const noexcept { return m_Swapchain; }
    };
}


class Model {
private:
    std::vector<Vertex> m_Vertices;
    std::vector<VertexIndex> m_Indices;

public:
    explicit Model(const char* filepath);

    const Vertex* Vertices() { return m_Vertices.data(); }

    const VertexIndex* Indices() { return m_Indices.data(); }

    VkDeviceSize VertexDataSize() { return sizeof(Vertex) * m_Vertices.size(); }

    VkDeviceSize IndexDataSize() { return sizeof(VertexIndex) * m_Indices.size(); }

    size_t IndexCount() { return m_Indices.size(); }

    size_t VertexCount() { return m_Vertices.size(); }
};

#endif //VULKAN_VULKAN_WRAPPERS_H
