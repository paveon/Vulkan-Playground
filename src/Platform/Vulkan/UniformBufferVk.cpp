#include "UniformBufferVk.h"


UniformBufferVk::UniformBufferVk(size_t objectByteSize) :
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()) {

    auto minOffset = m_Device.properties().limits.minUniformBufferOffsetAlignment;
    m_ObjectSize = objectByteSize;
    m_OffsetSize = roundUp(m_ObjectSize, minOffset);
    m_BufferSize = m_OffsetSize * m_Context.Swapchain().ImageCount();


    VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;


    m_Buffer = vk::Buffer(m_Device,
                        {m_Device.queueIndex(QueueFamily::GRAPHICS)},
                        m_BufferSize,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_Device, m_Buffer.data(), &memRequirements);
    auto memoryType = m_Device.getMemoryType(memRequirements.memoryTypeBits, memoryFlags);
    m_Memory = vk::DeviceMemory(m_Device, memoryType, memRequirements.size);
    m_Buffer.BindMemory(m_Memory.data(), 0);
}

void UniformBufferVk::SetData(uint32_t imageIndex, const void *data) {
    auto currentOffset = m_OffsetSize * imageIndex;

    void *memoryPtr = nullptr;
    vkMapMemory(m_Device, m_Memory.data(), currentOffset, m_ObjectSize, 0, &memoryPtr);
    memcpy(memoryPtr, data, m_ObjectSize);
    vkUnmapMemory(m_Device, m_Memory.data());
}
