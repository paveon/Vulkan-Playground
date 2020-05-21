#ifndef GAME_ENGINE_INDEX_BUFFER_H
#define GAME_ENGINE_INDEX_BUFFER_H

#include <memory>
#include <vulkan.h>

class IndexBuffer {
protected:
    IndexBuffer() = default;

public:
    virtual ~IndexBuffer() = default;

    static auto Create() -> std::unique_ptr<IndexBuffer>;

    virtual void UploadData(const void* data, uint64_t bytes) = 0;

    virtual void Bind(VkCommandBuffer cmdBuffer, uint64_t offset) const = 0;
};


#endif //GAMEENGINE_INDEXBUFFER_H
