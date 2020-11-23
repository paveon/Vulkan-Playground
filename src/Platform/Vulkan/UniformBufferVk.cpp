#include <Engine/Renderer/Renderer.h>
#include <Engine/Application.h>
#include "UniformBufferVk.h"
#include "GraphicsContextVk.h"


UniformBufferVk::UniformBufferVk(std::string name, size_t objectSize, bool perObject)
        : UniformBuffer(std::move(name)), m_PerObject(perObject) {
    auto &context = static_cast<GfxContextVk &>(Application::GetGraphicsContext());
    m_Device = &context.GetDevice();
    m_ImageCount = context.Swapchain().ImageCount();

    auto minOffset = m_Device->properties().limits.minUniformBufferOffsetAlignment;
    m_ObjectSize = objectSize;
    m_ObjectSizeAligned = roundUp(m_ObjectSize, minOffset);
}


void UniformBufferVk::Allocate(size_t objectCount) {
//    if (!m_PerObject && objectCount > 1) {
//        throw std::runtime_error("[UniformBufferVk::Allocate] Object count greater than 1 "
//                                 "is not allowed in non-dynamic uniform buffers");
//    }

    m_BufferSubSize = m_ObjectSizeAligned * objectCount;
    m_BufferSize = m_BufferSubSize * m_ImageCount;
    BufferAllocation metadata = Renderer::AllocateUniformBuffer(m_BufferSize);
    m_Memory = static_cast<VkDeviceMemory>(metadata.memory);
    m_Buffer = static_cast<VkBuffer>(metadata.handle);
    m_BaseOffset = metadata.offset;
}


void UniformBufferVk::SetData(const void *objectData, size_t objectCount, uint32_t offset) const {
    auto frameOffset = m_BaseOffset + m_BufferSubSize * Renderer::GetImageIndex();

    void *memoryPtr = nullptr;
    vkMapMemory(*m_Device, m_Memory, frameOffset + offset, m_BufferSubSize, 0, &memoryPtr);

    if (m_ObjectSize != m_ObjectSizeAligned) {
        /// Object data is tightly packed but we need incorporate padding due to memory requirements
        auto *dstPtr = (uint8_t *) memoryPtr;
        auto *srcPtr = (uint8_t *) objectData;
        for (size_t objectIdx = 0; objectIdx < objectCount; objectIdx++) {
            std::memcpy(dstPtr, srcPtr, m_ObjectSize);
            srcPtr += m_ObjectSize;
            dstPtr += m_ObjectSizeAligned;
        }
    } else {
        std::memcpy(memoryPtr, objectData, m_ObjectSizeAligned * objectCount);
    }

    vkUnmapMemory(*m_Device, m_Memory);
}
