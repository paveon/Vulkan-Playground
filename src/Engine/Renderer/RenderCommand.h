#ifndef GAME_ENGINE_RENDER_COMMAND_H
#define GAME_ENGINE_RENDER_COMMAND_H

#include <cstring>
#include <cstdint>
#include <glm/vec4.hpp>
#include <stdexcept>
#include "RendererAPI.h"
//#include "Material.h"
//#include "Mesh.h"

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 0.0f;
};

struct Scissor {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct DepthStencil {
    float depth;
    uint32_t stencil;
};

//struct BindVertexBufferPayload {
//    const VertexBuffer *vertexBuffer{};
//    uint64_t offset = 0;
//};
//
//struct BindIndexBufferPayload {
//    const IndexBuffer *indexBuffer{};
//    uint64_t offset = 0;
//};

struct SetDynamicOffsetPayload {
    uint32_t objectIndex;
};

struct DrawPayload {
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstVertex = 0;
    uint32_t firstInstance = 0;
};

struct DrawIndexedPayload {
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstIndex = 0;
    int32_t vertexOffset = 0;
    uint32_t firstInstance = 0;
};

class Mesh;
class Material;

struct RenderCommand {
    enum class Type : uint8_t {
        NONE,
        SET_VIEWPORT,
        SET_SCISSOR,
        SET_CLEAR_COLOR,
        SET_DEPTH_STENCIL,
        CLEAR,
        DRAW,
        DRAW_INDEXED,
//        BIND_VERTEX_BUFFER,
//        BIND_INDEX_BUFFER,
        BIND_MATERIAL,
        BIND_MESH,
        SET_UNIFORM_OFFSET,
    };

    Type m_Type = Type::NONE;
    uint8_t m_PayloadSize = 0;
    uint8_t m_Payload[64] = {};

    RenderCommand() = default;

    RenderCommand(Type type, size_t payloadSize, const void *payloadData) : m_Type(type), m_PayloadSize(payloadSize) {
        std::memcpy(m_Payload, payloadData, m_PayloadSize);
    }

    template<typename T>
    auto UnpackData() const -> T {
        T data;
        std::memcpy(&data, m_Payload, sizeof(T));
        return data;
    }

    static auto SetClearColor(const glm::vec4 &color) -> RenderCommand {
        return RenderCommand(Type::SET_CLEAR_COLOR, sizeof(glm::vec4), &color);
    }

    static auto SetDepthStencil(const DepthStencil &depthStencil) -> RenderCommand {
        return RenderCommand(Type::SET_DEPTH_STENCIL, sizeof(DepthStencil), &depthStencil);
    }

    static auto SetViewport(const Viewport &viewport) -> RenderCommand {
        return RenderCommand(Type::SET_VIEWPORT, sizeof(Viewport), &viewport);
    }

    static auto SetScissor(const Scissor &scissor) -> RenderCommand {
        return RenderCommand(Type::SET_SCISSOR, sizeof(Scissor), &scissor);
    }

    static auto Clear() -> RenderCommand {
        return RenderCommand(Type::CLEAR, 0, nullptr);
    }

//    static auto BindVertexBuffer(const VertexBuffer *vb, uint64_t offset) -> RenderCommand {
//        BindVertexBufferPayload payload{vb, offset};
//        return RenderCommand(Type::BIND_VERTEX_BUFFER, sizeof(BindVertexBufferPayload), &payload);
//    }
//
//    static auto BindIndexBuffer(const IndexBuffer *ib, uint64_t offset) -> RenderCommand {
//        BindIndexBufferPayload payload{ib, offset};
//        return RenderCommand(Type::BIND_INDEX_BUFFER, sizeof(BindIndexBufferPayload), &payload);
//    }

    static auto BindMaterial(const Material *material) -> RenderCommand {
        return RenderCommand(Type::BIND_MATERIAL, sizeof(void *), &material);
    }

    static auto BindMesh(const Mesh *mesh) -> RenderCommand {
        return RenderCommand(Type::BIND_MESH, sizeof(void *), &mesh);
    }

    static auto SetDynamicOffset(uint32_t objectIndex) -> RenderCommand {
        SetDynamicOffsetPayload payload{objectIndex};
        return RenderCommand(Type::SET_UNIFORM_OFFSET, sizeof(SetDynamicOffsetPayload), &payload);
    }

    static auto Draw(const DrawPayload& payload) -> RenderCommand {
//        DrawIndexedPayload payload{indexCount, firstIndex, vertexOffset};
        return RenderCommand(Type::DRAW, sizeof(DrawPayload), &payload);
    }

    static auto DrawIndexed(const DrawIndexedPayload& payload) -> RenderCommand {
//        DrawIndexedPayload payload{indexCount, firstIndex, vertexOffset};
        return RenderCommand(Type::DRAW_INDEXED, sizeof(DrawIndexedPayload), &payload);
    }
};


class RenderCommandQueue {
private:
    char *m_Buffer{};
    char *m_NextFree{};
    char *m_CurrentOffset{};
    size_t m_Capacity = 0;
    size_t m_FreeSpace = 0;
    size_t m_CommandCount = 0;
    size_t m_CommandIdx = 0;

    void Expand(size_t capacity) {
        size_t freeOffset = m_NextFree - m_Buffer;

        void *reallocatedPtr{};
        if (!(reallocatedPtr = std::realloc(m_Buffer, capacity)))
            throw std::runtime_error("[RenderCommandQueue] allocation failed");

        m_Buffer = static_cast<char *>(reallocatedPtr);
        m_NextFree = m_Buffer + freeOffset;
        m_FreeSpace += (capacity - m_Capacity);
        m_Capacity = capacity;
    }

public:
    explicit RenderCommandQueue(size_t size = 4096) : m_Capacity(size), m_FreeSpace(size) {
        if (!(m_Buffer = static_cast<char *>(std::malloc(m_Capacity))))
            throw std::runtime_error("[RenderCommandQueue] allocation failed");

        m_NextFree = m_Buffer;
    }

    ~RenderCommandQueue() {
        std::free(m_Buffer);
    }

    void AddCommand(const RenderCommand &command) {
        size_t totalSize = sizeof(RenderCommand::Type) + sizeof(RenderCommand::m_PayloadSize) + command.m_PayloadSize;
        if (m_FreeSpace < totalSize)
            Expand(m_Capacity * 2);

        std::memcpy(m_NextFree, &command.m_Type, sizeof(RenderCommand::Type));
        m_NextFree += sizeof(RenderCommand::Type);
        std::memcpy(m_NextFree, &command.m_PayloadSize, sizeof(RenderCommand::m_PayloadSize));
        m_NextFree += sizeof(RenderCommand::m_PayloadSize);
        std::memcpy(m_NextFree, command.m_Payload, command.m_PayloadSize);
        m_NextFree += command.m_PayloadSize;
        m_FreeSpace -= totalSize;
        ++m_CommandCount;
    }

    void Clear() {
        m_NextFree = m_Buffer;
        m_FreeSpace = m_Capacity;
        m_CommandIdx = 0;
        m_CommandCount = 0;
        m_CurrentOffset = nullptr;
    }

    auto GetNextCommand() -> RenderCommand * {
        static RenderCommand s_CommandView;

        if (!m_CurrentOffset) {
            m_CurrentOffset = m_Buffer;
            m_CommandIdx = 0;
        }

        if (m_CommandIdx >= m_CommandCount)
            return nullptr;

        std::memcpy(&s_CommandView.m_Type, m_CurrentOffset, sizeof(RenderCommand::Type));
        m_CurrentOffset += sizeof(RenderCommand::Type);
        std::memcpy(&s_CommandView.m_PayloadSize, m_CurrentOffset, sizeof(RenderCommand::m_PayloadSize));
        m_CurrentOffset += sizeof(RenderCommand::m_PayloadSize);
        std::memcpy(&s_CommandView.m_Payload, m_CurrentOffset, s_CommandView.m_PayloadSize);
        m_CurrentOffset += s_CommandView.m_PayloadSize;

        ++m_CommandIdx;
        return &s_CommandView;
    }
};


#endif //GAME_ENGINE_RENDER_COMMAND_H