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
inline void hash_combine(std::size_t &s, const T &v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6u) + (s >> 2u);
}


using VertexIndex = uint32_t;

struct Vertex {
    math::vec3 pos;
    math::vec3 color;
    math::vec2 texCoord;

    static auto getBindingDescription() -> VkVertexInputBindingDescription {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static auto getAttributeDescriptions() -> std::array<VkVertexInputAttributeDescription, 3> {
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

    auto operator==(const Vertex &other) const -> bool {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};


enum class QueueFamily {
    GRAPHICS = 0,
    PRESENT = 1,
    TRANSFER = 2,
    COUNT = 3
};


namespace vk {
    class LogicalDevice {
        VkPhysicalDevice m_Physical = nullptr;
        VkDevice m_Device = nullptr;
        VkSurfaceKHR m_Surface = nullptr;

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

        static const std::array<const char *, 1> s_RequiredExtensions;

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

        auto ptr() const noexcept -> const VkDevice* { return &m_Device; }

        auto data() const noexcept -> const VkDevice & { return m_Device; }

        auto GfxQueue() const -> VkQueue { return m_Queues[static_cast<size_t>(QueueFamily::GRAPHICS)]; }

        auto GfxQueueIdx() const -> uint32_t { return m_QueueIndices[static_cast<size_t>(QueueFamily::GRAPHICS)]; }

        auto PresentQueue() const -> VkQueue { return m_Queues[static_cast<size_t>(QueueFamily::PRESENT)]; }

        auto PresentQueueIdx() const -> uint32_t { return m_QueueIndices[static_cast<size_t>(QueueFamily::PRESENT)]; }

        auto TransferQueue() const -> VkQueue { return m_Queues[static_cast<size_t>(QueueFamily::TRANSFER)]; }

        auto TransferQueueIdx() const -> uint32_t { return m_QueueIndices[static_cast<size_t>(QueueFamily::TRANSFER)]; }

        auto queue(QueueFamily family) const -> VkQueue { return m_Queues[static_cast<size_t>(family)]; }

        auto queueIndex(QueueFamily family) const -> uint32_t { return m_QueueIndices[static_cast<size_t>(family)]; }

        auto getSurfaceFormats() const -> std::vector<VkSurfaceFormatKHR>;

        auto getSurfacePresentModes() const -> std::vector<VkPresentModeKHR>;
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

        auto ptr() const noexcept -> const VkCommandBuffer* { return &m_Buffer; }

        auto data() const noexcept -> const VkCommandBuffer& { return m_Buffer; }

        void Begin(VkCommandBufferUsageFlags flags = 0) const {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = flags;

            if (vkBeginCommandBuffer(m_Buffer, &beginInfo) != VK_SUCCESS)
                throw std::runtime_error("failed to begin command buffer recording!");
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
            vkQueueSubmit(cmdQueue, 1, &submitInfo, fence);
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

        CommandBuffers(VkDevice device, VkCommandPool pool) : CommandBuffers(device, pool, 1) {}

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

//        void Begin(VkCommandBufferUsageFlags flags = 0) const { Begin(0, flags); }
//
//        void Begin(size_t index, VkCommandBufferUsageFlags flags = 0) const {
//            operator[](index).Begin(flags);
//        }
//
//        void End() const { End(0); }
//
//        void End(size_t index) const {
//            operator[](index).End();
//        }
//
//        void Submit(VkQueue cmdQueue) const { Submit(0, cmdQueue); }
//
//        void Submit(size_t index, VkQueue cmdQueue) const {
//            operator[](index).Submit(cmdQueue);
//        }
//
//        void Submit(VkSubmitInfo &submitInfo, VkQueue cmdQueue, VkFence fence = nullptr) const {
//            Submit(0, submitInfo, cmdQueue, fence);
//        }
//
//        void Submit(size_t index, VkSubmitInfo &submitInfo, VkQueue cmdQueue, VkFence fence = nullptr) const {
//            operator[](index).Submit(submitInfo, cmdQueue, fence);
//        }
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

        void Move(Framebuffer &other) noexcept {
            m_Framebuffer = other.m_Framebuffer;
            m_Device = other.m_Device;
            other.m_Framebuffer = nullptr;
            other.m_Device = nullptr;
        }

        void Release() noexcept { if (m_Framebuffer) vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr); }

    public:
        ~Framebuffer() { Release(); }

        Framebuffer() = default;

        Framebuffer(VkDevice device, VkRenderPass renderPass, const VkExtent2D &extent,
                    const std::vector<VkImageView> &attachments) : m_Device(device) {
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

        void Move(Image &other) noexcept {
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

        Image(VkDevice device, const std::set<uint32_t> &queueIndices, const VkExtent2D &extent, uint32_t mipLevels,
              VkSampleCountFlagBits samples,
              VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage) : m_Device(device), m_Format(format),
                                                                                m_MipLevels(mipLevels),
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

        Image(VkDevice device, const VkImageCreateInfo &createInfo) : m_Device(device) {
            m_Format = createInfo.format;
            m_MipLevels = createInfo.mipLevels;
            Extent.width = createInfo.extent.width;
            Extent.height = createInfo.extent.height;
            if (vkCreateImage(m_Device, &createInfo, nullptr, &m_Image) != VK_SUCCESS)
                throw std::runtime_error("failed to create image!");
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

//        auto createView(VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) -> ImageView* {
//            return ImageView(m_Device, m_Image, format, m_MipLevels, aspectFlags);
//        }

        auto MipLevels() const -> uint32_t { return m_MipLevels; }

        void ChangeLayout(const CommandBuffer &cmdBuffer, VkImageLayout newLayout);

        void ChangeLayout(const CommandBuffer &cmdBuffer, VkImageLayout newLayout, VkPipelineStageFlags dstStage,
                          VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);

        void GenerateMipmaps(VkPhysicalDevice physicalDevice, const CommandBuffer &cmdBuffer);

        void BindMemory(VkDeviceMemory memory, VkDeviceSize offset) {
            if (vkBindImageMemory(m_Device, m_Image, memory, offset) != VK_SUCCESS)
                throw std::runtime_error("failed to bind memory to image!");
        }
    };


    inline ImageView::ImageView(VkDevice device, const Image &image, VkImageAspectFlags aspectFlags) : m_Device(
            device) {
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
        VkDeviceSize m_Size = 0;

        void Move(Buffer &other) noexcept {
            m_Buffer = other.m_Buffer;
            m_Device = other.m_Device;
            m_Size = other.m_Size;
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

        auto Size() const -> VkDeviceSize { return m_Size; }

        auto ptr() const noexcept -> const VkBuffer * { return &m_Buffer; }

        auto data() const noexcept -> const VkBuffer & { return m_Buffer; }

        void BindMemory(VkDeviceMemory memory, VkDeviceSize offset) {
            if (vkBindBufferMemory(m_Device, m_Buffer, memory, offset) != VK_SUCCESS)
                throw std::runtime_error("failed to bind memory to buffer!");
        }
    };


    class DeviceMemory {
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

    public:
        void *m_Mapping = nullptr;

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
            m_Mapping = nullptr;
            if (vkMapMemory(m_Device, m_Memory, offset, size, flags, &m_Mapping) != VK_SUCCESS) {
                throw std::runtime_error("failed to map device memory!");
            }
        }

        void UnmapMemory() {
            if (m_Mapping) {
                vkUnmapMemory(m_Device, m_Memory);
                m_Mapping = nullptr;
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

        DescriptorPool(VkDevice device, const std::vector<VkDescriptorPoolSize> &poolSizes, uint32_t maxSets,
                       VkDescriptorPoolCreateFlags flags = 0)
                : m_Device(
                device) {
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

        DescriptorSets(VkDevice device, VkDescriptorPool pool, const std::vector<VkDescriptorSetLayout> &layouts)
                : m_Device(device), m_Pool(pool) {
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

        DescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings) : m_Device(
                device) {
            VkDescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = bindings.size();
            layoutInfo.pBindings = bindings.data();

            if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
                throw std::runtime_error("failed to create descriptor set layout!");
        }

        DescriptorSetLayout(const DescriptorSetLayout &other) = delete;

        auto operator=(const DescriptorSetLayout &other) -> DescriptorSetLayout & = delete;

        DescriptorSetLayout(DescriptorSetLayout &&other) noexcept { Move(other); }

        auto operator=(DescriptorSetLayout &&other) noexcept -> DescriptorSetLayout & {
            if (this == &other) return *this;
            Release();
            Move(other);
            return *this;
        };

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
    private:
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

        static constexpr std::array<const char *, 2> s_ValidationLayers = {
                "VK_LAYER_KHRONOS_validation",
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
        VkFormat m_Format = VK_FORMAT_UNDEFINED;
        VkExtent2D m_Extent = {};
        std::vector<VkImage> m_Images;
        std::vector<ImageView> m_ImageViews;

        void Move(Swapchain &other) noexcept {
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

        auto ptr() const noexcept -> const VkSwapchainKHR * { return &m_Swapchain; }

        auto data() const noexcept -> const VkSwapchainKHR & { return m_Swapchain; }
    };
}


class Model {
private:
    std::vector<Vertex> m_Vertices;
    std::vector<VertexIndex> m_Indices;

public:
    explicit Model(const char *filepath);

    auto Vertices() -> const Vertex * { return m_Vertices.data(); }

    auto Indices() -> const VertexIndex * { return m_Indices.data(); }

    auto VertexDataSize() -> VkDeviceSize { return sizeof(Vertex) * m_Vertices.size(); }

    auto IndexDataSize() -> VkDeviceSize { return sizeof(VertexIndex) * m_Indices.size(); }

    auto IndexCount() -> size_t { return m_Indices.size(); }

    auto VertexCount() -> size_t { return m_Vertices.size(); }
};

#endif //VULKAN_VULKAN_WRAPPERS_H