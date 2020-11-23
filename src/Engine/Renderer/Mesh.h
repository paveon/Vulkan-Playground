#ifndef GAME_ENGINE_MESH_H
#define GAME_ENGINE_MESH_H

#include <memory>
#include <limits>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/material.h>
#include "Texture.h"
#include "Material.h"

template<class T>
inline void hash_combine(std::size_t &s, const T &v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6u) + (s >> 2u);
}


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};


struct MeshTexture {
    std::shared_ptr<Texture2D> texture;
    Texture2D::Type type;
};

class aiMesh;

class Mesh;

class MeshRenderer {
    friend class Mesh;

    const Mesh *m_Mesh{};
    MaterialInstance m_MaterialInstance;

    uint32_t m_ParentEntityID = 0;
    uint32_t m_MeshInstanceID = 0;

    explicit MeshRenderer(const Mesh *mesh, uint32_t instanceID, uint32_t parentEntityID) :
            m_Mesh(mesh), m_ParentEntityID(parentEntityID), m_MeshInstanceID(instanceID) {}

public:
    MeshRenderer() = default;

    MeshRenderer(const MeshRenderer &other) = delete;

    auto operator=(const MeshRenderer &other) -> MeshRenderer & = delete;

    MeshRenderer(MeshRenderer &&other) noexcept = default;

    auto operator=(MeshRenderer &&other) noexcept -> MeshRenderer & = default;

    void SetMaterialInstance(MaterialInstance material) {
        m_MaterialInstance = std::move(material);
    }

    auto GetMaterialInstance() const -> const MaterialInstance & { return m_MaterialInstance; }

    auto GetMaterial() const -> const Material * { return m_MaterialInstance.GetMaterial(); }

    auto GetMesh() const -> const Mesh * { return m_Mesh; }

    auto ParentEntityID() const { return m_ParentEntityID; }
};


class Mesh {
private:
    static uint32_t s_MeshIdCounter;

    std::vector<uint8_t> m_VertexData;
    std::vector<uint32_t> m_Indices;
    std::vector<uint8_t> m_VertexLayout;
    uint64_t m_VertexCount = 0;
    uint32_t m_VertexSize = 0;
    uint32_t m_InstanceCount = 0;
    uint32_t m_MeshID = 0;

    std::optional<uint32_t> m_AssimpMaterialIdx;

public:

    Mesh() : m_MeshID(s_MeshIdCounter++) {};

    Mesh(const aiMesh *sourceMesh, uint32_t assimpMaterialIdx);

    static auto Create(const aiMesh *sourceMesh, uint32_t assimpMaterialIdx) -> std::unique_ptr<Mesh> {
        return std::make_unique<Mesh>(sourceMesh, assimpMaterialIdx);
    }

    auto GetInstance(uint32_t parentEntityID) -> MeshRenderer {
        return MeshRenderer(this, m_InstanceCount++, parentEntityID);
    }

    static auto Cube() -> std::unique_ptr<Mesh>;

    static auto FromOBJ(const char *filepath) -> std::unique_ptr<Mesh>;

    auto VertexData() const -> const auto & { return m_VertexData; }

    auto Indices() const -> const auto & { return m_Indices; }

    template<typename T>
    auto Vertices() const -> const T * { return m_VertexData.data(); }

    auto VertexCount() const -> auto { return m_VertexCount; }

    auto VertexLayout() const -> const auto & { return m_VertexLayout; }

//    void SetMaterial(Material *material,
//                     const std::pair<uint32_t, uint32_t>& materialBinding,
//                     const std::unordered_map<Texture2D::Type, uint32_t>& textureIndices);

//    auto GetMaterial() const -> Material * { return m_Material; }

//    auto GetMaterialObjectIdx() const -> auto { return m_ObjectIdx; }

    auto AssimpMaterialIdx() const -> auto { return m_AssimpMaterialIdx.value(); }

    auto MeshID() const -> auto { return m_MeshID; }

    void StageData();
};


#endif //GAME_ENGINE_MESH_H
