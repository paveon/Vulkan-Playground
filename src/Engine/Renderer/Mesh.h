#ifndef GAME_ENGINE_MESH_H
#define GAME_ENGINE_MESH_H

#include <memory>
#include <limits>
#include <Engine/Renderer/Texture.h>

#include "Material.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"


template<class T>
inline void hash_combine(std::size_t &s, const T &v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6u) + (s >> 2u);
}


using VertexIndex = uint32_t;

struct Vertex {
    math::vec3 pos;
    math::vec3 color;
    math::vec2 texCoord;

//    static auto getBindingDescription() -> VkVertexInputBindingDescription {
//        VkVertexInputBindingDescription bindingDescription = {};
//        bindingDescription.binding = 0;
//        bindingDescription.stride = sizeof(Vertex);
//        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
//        return bindingDescription;
//    }
//
//    static auto getAttributeDescriptions() -> std::array<VkVertexInputAttributeDescription, 3> {
//        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
//        attributeDescriptions[0].binding = 0;
//        attributeDescriptions[0].location = 0;
//        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
//        attributeDescriptions[0].offset = 0;
//
//        attributeDescriptions[1].binding = 0;
//        attributeDescriptions[1].location = 1;
//        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
//        attributeDescriptions[1].offset = offsetof(Vertex, color);
//
//        attributeDescriptions[2].binding = 0;
//        attributeDescriptions[2].location = 2;
//        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
//        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
//
//        return attributeDescriptions;
//    }

    auto operator==(const Vertex &other) const -> bool {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};


class Mesh {
private:
    std::shared_ptr<Material> m_Material{};
//    std::unique_ptr<VertexBuffer> m_VertexBuffer{};
//    std::unique_ptr<IndexBuffer> m_IndexBuffer{};
    std::vector<Vertex> m_Vertices;
    std::vector<VertexIndex> m_Indices;

public:
    size_t m_ResourceID = std::numeric_limits<size_t>::max();
//    Mesh(std::unique_ptr<VertexBuffer> vertexBuffer, std::unique_ptr<IndexBuffer> indexBuffer) :
//            m_VertexBuffer(std::move(vertexBuffer)), m_IndexBuffer(std::move(indexBuffer)) {}

    explicit Mesh(const char *objFilepath);

    auto Vertices() const -> const Vertex * { return m_Vertices.data(); }

    auto Indices() const -> const VertexIndex * { return m_Indices.data(); }

    auto VertexDataSize() const -> auto { return sizeof(Vertex) * m_Vertices.size(); }

    auto IndexDataSize() const -> auto { return sizeof(VertexIndex) * m_Indices.size(); }

    auto IndexCount() const -> auto { return m_Indices.size(); }

    auto VertexCount() const -> auto { return m_Vertices.size(); }

//    auto VertexBuffer() const -> const VertexBuffer & { return *m_VertexBuffer; }
//
//    auto IndexBuffer() const -> const IndexBuffer & { return *m_IndexBuffer; }
//
    void SetMaterial(const std::shared_ptr<Material>& material) { m_Material = material; }

    auto GetMaterial() const -> Material& { return *m_Material; }

    void StageData();
};


#endif //GAME_ENGINE_MESH_H
