#ifndef GAME_ENGINE_UNIFORM_BUFFER_H
#define GAME_ENGINE_UNIFORM_BUFFER_H

#include <memory>

class UniformBuffer {
protected:
    UniformBuffer() = default;

    uint64_t m_BufferSize = 0;
    uint32_t m_ObjectSize = 0;
    uint32_t m_ObjectSizeAligned = 0;

public:
    virtual ~UniformBuffer() = default;

    auto ObjectSize() const -> auto { return m_ObjectSize; }

    auto ObjectSizeAligned() const -> auto { return m_ObjectSizeAligned; }

    auto Size() const -> auto { return m_BufferSize; }

//    virtual auto VkHandle() const -> const void* { return nullptr; }

    virtual void SetData(const void *objectData, size_t objectCount) = 0;

    static auto Create(size_t objectSize, size_t objectCount) -> std::unique_ptr<UniformBuffer>;
};


#endif //GAME_ENGINE_UNIFORM_BUFFER_H
