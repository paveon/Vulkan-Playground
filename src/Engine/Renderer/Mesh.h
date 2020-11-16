#ifndef GAME_ENGINE_MESH_H
#define GAME_ENGINE_MESH_H

#include <memory>
#include <limits>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/material.h>
#include "Texture.h"


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
    enum class Type {
        DIFFUSE,
        SPECULAR
    };

    std::shared_ptr<Texture2D> texture;
    Type type;
};

class aiMesh;

class aiMaterial;

class Material;

class Mesh {
public:
//    struct VertexAttribute {
//        uint8_t size;
//    };

private:
    std::vector<uint8_t> m_VertexData;
    std::vector<uint32_t> m_Indices;
    std::vector<uint8_t> m_VertexLayout;
    Material *m_Material{};
    uint32_t m_ObjectIdx = 0;
    uint32_t m_AssimpMaterialIdx = 0;
    size_t m_VertexCount = 0;
    uint32_t m_VertexSize = 0;

//    void loadMaterialTextures(const aiMaterial *mat, const std::vector<aiTextureType> &types);

public:
    size_t m_ResourceID = std::numeric_limits<size_t>::max();

    static auto Create(const aiMesh *sourceMesh) -> std::unique_ptr<Mesh>;

    static auto Cube() -> std::unique_ptr<Mesh>;

    static auto FromOBJ(const char *filepath) -> std::unique_ptr<Mesh>;

    auto VertexData() const -> const auto & { return m_VertexData; }

    auto Indices() const -> const auto & { return m_Indices; }

    template<typename T>
    auto Vertices() const -> const T * { return m_VertexData.data(); }

    auto VertexCount() const -> auto { return m_VertexCount; }

    auto VertexLayout() const -> const auto & { return m_VertexLayout; }

    void SetMaterial(Material *material,
                     const std::pair<uint32_t, uint32_t>& materialBinding,
                     const std::unordered_map<MeshTexture::Type, uint32_t>& textureIndices);

    auto GetMaterial() const -> Material * { return m_Material; }

    auto GetMaterialObjectIdx() const -> auto { return m_ObjectIdx; }

    auto AssimpMaterialIdx() const -> auto { return m_AssimpMaterialIdx; }

    void StageData();
};


class MeshInstance {
    friend class Mesh;

    Mesh* m_Mesh{};
    Material *m_Material{};
    uint32_t m_MaterialObjectIdx = 0;
};


#endif //GAME_ENGINE_MESH_H
