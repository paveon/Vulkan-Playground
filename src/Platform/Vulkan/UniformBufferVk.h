#ifndef GAME_ENGINE_UNIFORM_BUFFER_VK_H
#define GAME_ENGINE_UNIFORM_BUFFER_VK_H

#include <Engine/Renderer/UniformBuffer.h>
#include <Engine/Renderer/vulkan_wrappers.h>
#include <Engine/Application.h>


class UniformBufferVk : public UniformBuffer {
    GfxContextVk &m_Context;
    Device &m_Device;
    vk::DeviceMemory m_Memory;
    vk::Buffer m_Data;
    VkDeviceSize m_OffsetSize;

public:
    explicit UniformBufferVk(size_t objectByteSize);

    auto VkHandle() const -> const void* override { return m_Data.data(); }

    void SetData(uint32_t imageIndex, const void *data) override;
};


#endif //GAME_ENGINE_UNIFORM_BUFFER_VK_H
