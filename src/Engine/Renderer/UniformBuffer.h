#ifndef GAME_ENGINE_UNIFORM_BUFFER_H
#define GAME_ENGINE_UNIFORM_BUFFER_H

#include <memory>

class UniformBuffer {
protected:
    UniformBuffer() = default;

    size_t m_BufferSize = 0;
    size_t m_ObjectSize = 0;

public:
    virtual ~UniformBuffer() = default;

    auto ObjectSize() const -> size_t { return m_ObjectSize; }

    auto Size() const -> size_t { return m_BufferSize; }

    virtual auto VkHandle() const -> const void* { return nullptr; }

    virtual void SetData(uint32_t imageIndex, const void *data) = 0;

    static auto Create(size_t objectByteSize) -> std::unique_ptr<UniformBuffer>;
};


#endif //GAME_ENGINE_UNIFORM_BUFFER_H
