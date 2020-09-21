#ifndef GAME_ENGINE_VERTEX_BUFFER_H
#define GAME_ENGINE_VERTEX_BUFFER_H

#include <memory>
#include <vulkan.h>


class VertexBuffer {
protected:
    VertexBuffer() = default;

    uint64_t m_VertexCount = 0;

public:
    virtual ~VertexBuffer() = default;

    static auto Create() -> std::unique_ptr<VertexBuffer>;

    auto VertexCount() const -> uint64_t { return m_VertexCount; }

//    void Upload(const void *data, uint64_t bytes, uint64_t vertexCount) {
//
//    }

    virtual void StageData(const void *data, uint64_t bytes, uint64_t vertexCount) = 0;

    virtual void UploadData(const void *data, uint64_t bytes, uint64_t vertexCount) = 0;

    virtual void Bind(VkCommandBuffer cmdBuffer, uint64_t offset) const = 0;
};


#endif //GAME_ENGINE_VERTEX_BUFFER_H
