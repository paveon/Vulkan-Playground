#ifndef GAME_ENGINE_VERTEX_BUFFER_VK_H
#define GAME_ENGINE_VERTEX_BUFFER_VK_H

#include <Engine/Renderer/VertexBuffer.h>
#include <Engine/Renderer/vulkan_wrappers.h>
#include <Engine/Application.h>
#include "GraphicsContextVk.h"


class VertexBufferVk : public VertexBuffer {
    GfxContextVk &m_Context;
    Device &m_Device;
    DeviceBuffer m_Buffer;
    vk::Buffer* m_BufferHandle = nullptr;
    VkDeviceSize m_BufferOffset = 0;

public:
    VertexBufferVk() : m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
                       m_Device(m_Context.GetDevice()) {}

    void StageData(const void *data, uint64_t bytes, uint64_t vertexCount) override;

    void UploadData(const void *data, uint64_t bytes, uint64_t vertexCount) override;

    void Bind(VkCommandBuffer cmdBuffer, uint64_t offset) const override;
};


#endif //GAME_ENGINE_VERTEX_BUFFER_VK_H
