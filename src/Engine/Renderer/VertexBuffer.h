#ifndef GAME_ENGINE_VERTEX_BUFFER_H
#define GAME_ENGINE_VERTEX_BUFFER_H

#include <memory>
#include <vulkan.h>

class VertexBuffer {
protected:
    VertexBuffer() = default;

public:
    virtual ~VertexBuffer() = default;

    static auto Create() -> std::unique_ptr<VertexBuffer>;

    virtual void UploadData(const void* data, uint64_t bytes) = 0;

    virtual void Bind(VkCommandBuffer cmdBuffer, uint64_t offset) const = 0;
};


#endif //GAME_ENGINE_VERTEX_BUFFER_H
