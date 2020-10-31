#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "Mesh.h"
#include "Renderer.h"

#include <fstream>
#include <tiny_obj_loader.h>
#include <stb_image.h>
#include <unordered_map>
#include <unordered_set>
#include <functional>


namespace std {
    template<>
    struct hash<math::vec2> {
        auto operator()(const math::vec2 &v) const -> size_t {
            size_t res = 0;
            hash_combine(res, v.x);
            hash_combine(res, v.y);
            return res;
        }
    };

    template<>
    struct hash<math::vec3> {
        auto operator()(const math::vec3 &v) const -> size_t {
            size_t res = 0;
            hash_combine(res, v.x);
            hash_combine(res, v.y);
            hash_combine(res, v.z);
            return res;
        }
    };

    template<>
    struct hash<math::vec4> {
        auto operator()(const math::vec4 &v) const -> size_t {
            size_t res = 0;
            hash_combine(res, v.x);
            hash_combine(res, v.y);
            hash_combine(res, v.z);
            hash_combine(res, v.w);
            return res;
        }
    };

    template<>
    struct hash<Vertex> {
        auto operator()(const Vertex &vertex) const -> size_t {
            size_t res = 0;
            hash_combine(res, vertex.pos);
            hash_combine(res, vertex.color);
            hash_combine(res, vertex.texCoord);
            return res;
        }
    };

    template<>
    struct hash<tinyobj::index_t> {
        auto operator()(const tinyobj::index_t &index) const -> size_t {
            size_t res = 0;
            hash_combine(res, index.normal_index);
            hash_combine(res, index.texcoord_index);
            hash_combine(res, index.vertex_index);
            return res;
        }
    };

    template<>
    struct equal_to<tinyobj::index_t> {
        constexpr auto operator()(const tinyobj::index_t &lhs, const tinyobj::index_t &rhs) const -> bool {
            return lhs.vertex_index == rhs.vertex_index
                   && lhs.texcoord_index == rhs.texcoord_index
                   && lhs.normal_index == rhs.normal_index;
        }
    };
}


Mesh::Mesh(const char *objFilepath) {
    std::ifstream dumpInput(std::string(objFilepath) + ".dump", std::ios::in | std::ios::binary);
    if (dumpInput) {
        /* Read header */
        std::size_t vertexCount = 0;
        dumpInput.read(reinterpret_cast<char *>(&vertexCount), sizeof(vertexCount));
        m_Vertices.resize(vertexCount);
        dumpInput.read(reinterpret_cast<char *>(m_Vertices.data()), sizeof(Vertex) * vertexCount);

        std::size_t indexCount = 0;
        dumpInput.read(reinterpret_cast<char *>(&indexCount), sizeof(indexCount));
        m_Indices.resize(indexCount);
        dumpInput.read(reinterpret_cast<char *>(m_Indices.data()), sizeof(VertexIndex) * indexCount);

        dumpInput.close();
    } else {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFilepath)) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<tinyobj::index_t, uint32_t> uniqueIndices;
        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                uniqueIndices.find(index);
                auto it = uniqueIndices.find(index);
                if (it == uniqueIndices.cend()) {
                    it = uniqueIndices.insert(std::make_pair(index, m_Vertices.size())).first;

                    Vertex vertex = {};
                    vertex.pos = {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                    };
                    vertex.texCoord = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };

                    vertex.color = {1.0f, 1.0f, 1.0f};

                    m_Vertices.push_back(vertex);
                }
                m_Indices.push_back(it->second);
            }
        }

        std::ofstream dumpOutput(std::string(objFilepath) + ".dump", std::ios::out | std::ios::binary);
        if (dumpOutput) {
            std::size_t vertexCount = m_Vertices.size();
            std::size_t indexCount = m_Indices.size();
            dumpOutput.write(reinterpret_cast<char *>(&vertexCount), sizeof(vertexCount));
            dumpOutput.write(reinterpret_cast<char *>(m_Vertices.data()), VertexDataSize());
            dumpOutput.write(reinterpret_cast<char *>(&indexCount), sizeof(std::size_t));
            dumpOutput.write(reinterpret_cast<char *>(m_Indices.data()), IndexDataSize());
            dumpOutput.close();
        }
    }
}

void Mesh::StageData() {
    Renderer::StageMesh(this);
//    Renderer::StageData()
}
