#ifndef GAME_ENGINE_UNIFORM_BUFFER_VK_H
#define GAME_ENGINE_UNIFORM_BUFFER_VK_H

#include <Engine/Renderer/UniformBuffer.h>
#include <Engine/Renderer/vulkan_wrappers.h>
#include <Engine/Application.h>
#include "GraphicsContextVk.h"


class UniformBufferVk : public UniformBuffer {
//    GfxContextVk &m_Context;
    Device *m_Device = nullptr;
//    vk::DeviceMemory m_Memory;
//    vk::Buffer m_Buffer;

    VkDeviceMemory m_Memory = nullptr;
    VkBuffer m_Buffer = nullptr;
    VkDeviceSize m_BufferSubSize = 0;
    VkDeviceSize m_BaseOffset = 0;
    uint32_t m_ImageCount = 0;
    bool m_Dynamic = false;

public:
    explicit UniformBufferVk(size_t objectSize, bool dynamic);

    void Allocate(size_t objectCount);

    auto VkHandle() const -> auto { return m_Buffer; }

    auto SubSize() const -> auto { return m_BufferSubSize; }

    auto BaseOffset() const -> auto { return m_BaseOffset; }

    auto IsDynamic() const -> bool { return m_Dynamic; }

    void SetData(const void *objectData, size_t objectCount) override;
};


#endif //GAME_ENGINE_UNIFORM_BUFFER_VK_H
