#ifndef GAME_ENGINE_UNIFORM_BUFFER_VK_H
#define GAME_ENGINE_UNIFORM_BUFFER_VK_H

#include <Engine/Renderer/UniformBuffer.h>
#include <Engine/Renderer/vulkan_wrappers.h>

class Device;

class UniformBufferVk : public UniformBuffer {
    Device *m_Device = nullptr;
    VkDeviceMemory m_Memory = nullptr;
    VkBuffer m_Buffer = nullptr;
    VkDeviceSize m_BufferSubSize = 0;
    VkDeviceSize m_BaseOffset = 0;
    uint32_t m_ImageCount = 0;
    bool m_PerObject = false;

public:
    explicit UniformBufferVk(std::string name, size_t objectSize, bool perObject);

    void Allocate(size_t objectCount);

    auto VkHandle() const -> auto { return m_Buffer; }

    auto SubSize() const -> auto { return m_BufferSubSize; }

    auto BaseOffset() const -> auto { return m_BaseOffset; }

    auto IsDynamic() const -> bool { return m_PerObject; }

    void SetData(const void *objectData, size_t objectCount, uint32_t offset) const override;
};


#endif //GAME_ENGINE_UNIFORM_BUFFER_VK_H
