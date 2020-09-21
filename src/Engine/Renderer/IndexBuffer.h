#ifndef GAME_ENGINE_INDEX_BUFFER_H
#define GAME_ENGINE_INDEX_BUFFER_H

#include <memory>
#include <vulkan.h>

class IndexBuffer {
protected:
    IndexBuffer() = default;

    uint64_t m_IndexCount = 0;

public:
    virtual ~IndexBuffer() = default;

    static auto Create() -> std::unique_ptr<IndexBuffer>;

    auto IndexCount() const -> uint64_t { return m_IndexCount; }

    virtual void StageData(const void *data, uint64_t bytes, uint64_t indexCount) = 0;

    virtual void UploadData(const void *data, uint64_t bytes, uint64_t indexCount) = 0;

    virtual void Bind(VkCommandBuffer cmdBuffer, uint64_t offset) const = 0;
};


#endif //GAMEENGINE_INDEXBUFFER_H
