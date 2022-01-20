#ifndef GAME_ENGINE_UNIFORM_BUFFER_H
#define GAME_ENGINE_UNIFORM_BUFFER_H

#include <string>
#include <memory>

class UniformBuffer {
protected:
    std::string m_Name;
    uint64_t m_BufferSize = 0;
    uint32_t m_ObjectSize = 0;
    uint32_t m_ObjectSizeAligned = 0;

    explicit UniformBuffer(std::string name) : m_Name(std::move(name)) {}

public:
    static auto Create(std::string name, size_t objectSize, size_t objectCount) -> std::unique_ptr<UniformBuffer>;

    virtual ~UniformBuffer() = default;

    auto ObjectSize() const -> auto { return m_ObjectSize; }

    auto ObjectSizeAligned() const -> auto { return m_ObjectSizeAligned; }

    auto Size() const -> auto { return m_BufferSize; }

//    virtual auto VkHandle() const -> const void* { return nullptr; }

    virtual void SetData(const void *objectData, size_t objectCount, uint32_t offset) const = 0;

    virtual void SetMemberData(const void *memberData, uint32_t memberBytes, uint32_t memberOffset) const = 0;
};


#endif //GAME_ENGINE_UNIFORM_BUFFER_H
