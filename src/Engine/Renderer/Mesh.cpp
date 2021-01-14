#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include "Material.h"
#include "Mesh.h"
#include "Renderer.h"

#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <tiny_obj_loader.h>
#include <stb_image.h>
#include <unordered_map>
#include <functional>
#include <numeric>
#include <iostream>
#include <unordered_set>


uint32_t Mesh::s_MeshIdCounter = 0;

const std::array<glm::vec3, 36> Mesh::s_CubeVertexPositions{
        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f),

        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(-0.5f, 0.5f, 0.5f),

        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),

        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(0.5f, -0.5f, 0.5f),

        glm::vec3(0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5),
        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),

        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, -0.5f, -0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, 0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
};


namespace std {
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


void Mesh::StageData() {
    Renderer::StageMesh(this);
}


auto Mesh::Cube() -> std::unique_ptr<Mesh> {
    auto mesh(std::make_unique<Mesh>());
    static std::array<Vertex, 36> vertices{
            // TOP
            Vertex{
                    glm::vec3(-0.5f, -0.5f, 0.5f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, -0.5f, 0.5f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, 0.5f, 0.5f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(0.5f, 0.5f, 0.5f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, 0.5f, 0.5f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, -0.5f, 0.5f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
            },

            Vertex{
                    glm::vec3(-0.5f, 0.5f, 0.5f),
                    glm::vec3(-1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, 0.5f, -0.5f),
                    glm::vec3(-1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, -0.5f, -0.5f),
                    glm::vec3(-1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, -0.5f, -0.5f),
                    glm::vec3(-1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, -0.5f, 0.5f),
                    glm::vec3(-1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, 0.5f, 0.5f),
                    glm::vec3(-1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },

            Vertex{
                    glm::vec3(0.5f, 0.5f, 0.5f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(0.5f, 0.5f, -0.5f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(0.5f, -0.5f, -0.5f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, -0.5f, -0.5f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, -0.5f, 0.5f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, 0.5f, 0.5f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },

            Vertex{
                    glm::vec3(0.5f, -0.5f, 0.5f),
                    glm::vec3(0.0f, -1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(0.5f, -0.5f, -0.5f),
                    glm::vec3(0.0f, -1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, -0.5f, -0.5f),
                    glm::vec3(0.0f, -1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, -0.5f, -0.5f),
                    glm::vec3(0.0f, -1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, -0.5f, 0.5f),
                    glm::vec3(0.0f, -1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, -0.5f, 0.5f),
                    glm::vec3(0.0f, -1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },

            Vertex{
                    glm::vec3(0.5f, 0.5f, 0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(0.5f, 0.5f, -0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, 0.5f, -0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, 0.5f, -0.5),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, 0.5f, 0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, 0.5f, 0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },

            //BOTTOM
            Vertex{
                    glm::vec3(-0.5f, -0.5f, -0.5f),
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, -0.5f, -0.5f),
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, 0.5f, -0.5f),
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(0.5f, 0.5f, -0.5f),
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, 0.5f, -0.5f),
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
            },
            Vertex{
                    glm::vec3(-0.5f, -0.5f, -0.5f),
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
            },
    };

    mesh->m_VertexLayout.push_back(sizeof(Vertex::position));
    mesh->m_VertexLayout.push_back(sizeof(Vertex::normal));
    mesh->m_VertexLayout.push_back(sizeof(Vertex::tangent));
    mesh->m_VertexLayout.push_back(sizeof(Vertex::bitangent));
    mesh->m_VertexLayout.push_back(sizeof(Vertex::texCoords));

    mesh->m_VertexCount = vertices.size();
    mesh->m_VertexSize = sizeof(Vertex);
    mesh->m_VertexData.resize(sizeof(Vertex) * vertices.size());
    std::memcpy(mesh->m_VertexData.data(), vertices.data(), mesh->m_VertexData.size());

    return mesh;
}


auto Mesh::Quad() -> std::unique_ptr<Mesh> {
    auto mesh(std::make_unique<Mesh>());

    static std::array<Vertex, 6> vertices{
            Vertex{
                    glm::vec3(-0.5f, 0.0f, -0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f)
            },
            Vertex{
                    glm::vec3(0.5f, 0.0f, -0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(1.0f, 0.0f)},
            Vertex{
                    glm::vec3(0.5f, 0.0f, 0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(1.0f, 1.0f)},
            Vertex{
                    glm::vec3(0.5f, 0.0f, 0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(1.0f, 1.0f)},
            Vertex{
                    glm::vec3(-0.5f, 0.0f, 0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(0.0f, 1.0f)},
            Vertex{
                    glm::vec3(-0.5f, 0.0f, -0.5f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f)
            },
    };

    mesh->m_VertexLayout.push_back(sizeof(Vertex::position));
    mesh->m_VertexLayout.push_back(sizeof(Vertex::normal));
    mesh->m_VertexLayout.push_back(sizeof(Vertex::tangent));
    mesh->m_VertexLayout.push_back(sizeof(Vertex::bitangent));
    mesh->m_VertexLayout.push_back(sizeof(Vertex::texCoords));

    mesh->m_VertexCount = vertices.size();
    mesh->m_VertexSize = sizeof(Vertex);
    mesh->m_VertexData.resize(sizeof(Vertex) * vertices.size());
    std::memcpy(mesh->m_VertexData.data(), vertices.data(), mesh->m_VertexData.size());

    return mesh;
}


auto Mesh::Sphere() -> std::unique_ptr<Mesh> {
    auto mesh(std::make_unique<Mesh>());
    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;


    mesh->m_VertexSize = sizeof(Vertex);
    mesh->m_VertexCount = (X_SEGMENTS + 1) * (Y_SEGMENTS + 1);
    mesh->m_VertexData.resize(mesh->m_VertexCount * mesh->m_VertexSize);
    mesh->m_VertexLayout = {
            sizeof(Vertex::position),
            sizeof(Vertex::normal),
            sizeof(Vertex::tangent),
            sizeof(Vertex::bitangent),
            sizeof(Vertex::texCoords)
    };

    auto *vertexPtr = reinterpret_cast<Vertex *>(mesh->m_VertexData.data());
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
    {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI_F) * std::sin(ySegment * PI_F);
            float yPos = std::cos(ySegment * PI_F);
            float zPos = std::sin(xSegment * 2.0f * PI_F) * std::sin(ySegment * PI_F);

            vertexPtr->position = glm::vec3(xPos, yPos, zPos);
            vertexPtr->normal = glm::vec3(xPos, yPos, zPos);
            vertexPtr->texCoords = glm::vec2(xSegment, ySegment);
            vertexPtr++;
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        if (!oddRow) {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
                mesh->m_Indices.push_back(y * (X_SEGMENTS + 1) + x);
                mesh->m_Indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else {
            for (int x = X_SEGMENTS; x >= 0; --x) {
                mesh->m_Indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                mesh->m_Indices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }

    vertexPtr = reinterpret_cast<Vertex *>(mesh->m_VertexData.data());
    for (size_t i = 0; i < (mesh->m_Indices.size() - 2); i++) {
        uint32_t i1 = mesh->m_Indices[i];
        uint32_t i2 = mesh->m_Indices[i + 1];
        uint32_t i3 = mesh->m_Indices[i + 2];
        Vertex* v1 = &vertexPtr[i1];
        Vertex* v2 = &vertexPtr[i2];
        Vertex* v3 = &vertexPtr[i3];

        glm::vec3 edge1 = v2->position - v1->position;
        glm::vec3 edge2 = v3->position - v1->position;
        glm::vec2 deltaUV1 = v2->texCoords - v1->texCoords;
        glm::vec2 deltaUV2 = v3->texCoords - v1->texCoords;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
        glm::vec3 bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

        v1->tangent = tangent;
        v2->tangent = tangent;
        v3->tangent = tangent;

        v1->bitangent = bitangent;
        v2->bitangent = bitangent;
        v3->bitangent = bitangent;
    }

    return mesh;
}


auto Mesh::FromOBJ(const char *filepath) -> std::unique_ptr<Mesh> {
    auto mesh(std::make_unique<Mesh>());
    auto &vertexData = mesh->m_VertexData;
    auto &indices = mesh->m_Indices;
    std::ifstream dumpFile(std::string(filepath) + ".dump", std::ios::in | std::ios::binary);
    if (dumpFile) {
        /* Read vertex layout */
        size_t layoutSize = 0;
        dumpFile.read((char *) &layoutSize, sizeof(layoutSize));
        mesh->m_VertexLayout.resize(layoutSize);
        dumpFile.read((char *) mesh->m_VertexLayout.data(), layoutSize);

        /* Read vertex data */
        dumpFile.read((char *) &mesh->m_VertexSize, sizeof(mesh->m_VertexSize));
        dumpFile.read((char *) &mesh->m_VertexCount, sizeof(mesh->m_VertexCount));
        mesh->m_VertexData.resize(mesh->m_VertexCount * mesh->m_VertexSize);
        dumpFile.read((char *) mesh->m_VertexData.data(), mesh->m_VertexData.size());

        /* Read index data */
        size_t indexCount = 0;
        dumpFile.read((char *) &indexCount, sizeof(indexCount));
        mesh->m_Indices.resize(indexCount);
        dumpFile.read((char *) mesh->m_Indices.data(), sizeof(uint32_t) * indexCount);

        dumpFile.close();
    } else {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath)) {
            throw std::runtime_error(warn + err);
        }

        mesh->m_VertexLayout.push_back(sizeof(Vertex::position));
        mesh->m_VertexLayout.push_back(sizeof(Vertex::normal));
        mesh->m_VertexLayout.push_back(sizeof(Vertex::texCoords));
        uint32_t vertexSize = sizeof(Vertex);
        mesh->m_VertexSize = vertexSize;

        size_t vertexCount = 0;
        vertexData.reserve((attrib.vertices.size() / 3) * vertexSize);

        std::unordered_map<tinyobj::index_t, uint32_t> uniqueIndices;
        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                auto it = uniqueIndices.find(index);
                if (it == uniqueIndices.cend()) {
                    it = uniqueIndices.insert(std::make_pair(index, vertexCount)).first;

                    vertexData.resize(vertexData.size() + vertexSize);
                    auto *vertexPtr = reinterpret_cast<Vertex *>(&vertexData[vertexCount * vertexSize]);
                    auto *pos_ptr = &attrib.vertices[3 * index.vertex_index];
                    std::memcpy(&vertexPtr->position.x, pos_ptr, sizeof(Vertex::position));

                    if (!attrib.normals.empty()) {
                        auto *normal_ptr = &attrib.normals[3 * index.normal_index];
                        std::memcpy(&vertexPtr->normal.x, normal_ptr, sizeof(Vertex::normal));
                    }
                    if (!attrib.texcoords.empty()) {
                        auto *texcoord_ptr = &attrib.texcoords[2 * index.texcoord_index];
                        std::memcpy(&vertexPtr->texCoords.x, texcoord_ptr, sizeof(Vertex::texCoords));
                    }

                    vertexCount++;
                }
                indices.push_back(it->second);
            }
            std::cout << "Unique indices: " << uniqueIndices.size() << std::endl;

            if (attrib.normals.empty()) {
                // Try to estimate normals from faces
                auto *vertexPtr = reinterpret_cast<Vertex *>(vertexData.data());
                std::vector<glm::vec3> normals(vertexCount);
                for (size_t i = 0; i < indices.size(); i += 3) {
                    auto i1 = indices[i];
                    auto i2 = indices[i + 1];
                    auto i3 = indices[i + 2];
                    Vertex *v1 = &vertexPtr[i1];
                    Vertex *v2 = &vertexPtr[i2];
                    Vertex *v3 = &vertexPtr[i3];
                    glm::vec3 normal = glm::normalize(glm::cross(
                            glm::vec3(v2->position) - glm::vec3(v1->position),
                            glm::vec3(v3->position) - glm::vec3(v1->position)
                    ));
                    normals[i1] += normal;
                    normals[i2] += normal;
                    normals[i3] += normal;
                }
                for (size_t i = 0; i < vertexCount; i++) {
                    vertexPtr[i].normal = glm::normalize(normals[i]);
                }
            }
        }

        std::ofstream dumpOutput(std::string(filepath) + ".dump", std::ios::out | std::ios::binary);
        if (dumpOutput) {
            size_t indexCount = indices.size();
            size_t layoutSize = mesh->m_VertexLayout.size() * sizeof(uint8_t);
            dumpOutput.write((char *) &layoutSize, sizeof(layoutSize));
            dumpOutput.write((char *) mesh->m_VertexLayout.data(), layoutSize);
            dumpOutput.write((char *) &vertexSize, sizeof(vertexSize));
            dumpOutput.write((char *) (&vertexCount), sizeof(vertexCount));
            dumpOutput.write((char *) vertexData.data(), vertexData.size());
            dumpOutput.write((char *) (&indexCount), sizeof(indexCount));
            dumpOutput.write((char *) indices.data(), sizeof(uint32_t) * indices.size());
            dumpOutput.close();
        }
    }

    return mesh;
}


Mesh::Mesh(const aiMesh *sourceMesh, uint32_t assimpMaterialIdx)
        : m_VertexSize(sizeof(Vertex)),
          m_MeshID(s_MeshIdCounter++),
          m_AssimpMaterialIdx(assimpMaterialIdx) {
    m_VertexLayout.push_back(sizeof(Vertex::position));
    m_VertexLayout.push_back(sizeof(Vertex::normal));
    m_VertexLayout.push_back(sizeof(Vertex::texCoords));
    uint32_t vertexSize = sizeof(Vertex);

    m_VertexData.resize(sourceMesh->mNumVertices * vertexSize);
    auto *vertexPtr = reinterpret_cast<Vertex *>(m_VertexData.data());
    for (unsigned int i = 0; i < sourceMesh->mNumVertices; i++) {
        vertexPtr->position.x = sourceMesh->mVertices[i].x;
        vertexPtr->position.y = sourceMesh->mVertices[i].y;
        vertexPtr->position.z = sourceMesh->mVertices[i].z;
        vertexPtr->normal.x = sourceMesh->mNormals[i].x;
        vertexPtr->normal.y = sourceMesh->mNormals[i].y;
        vertexPtr->normal.z = sourceMesh->mNormals[i].z;
        vertexPtr->tangent.x = sourceMesh->mTangents[i].x;
        vertexPtr->tangent.y = sourceMesh->mTangents[i].y;
        vertexPtr->tangent.z = sourceMesh->mTangents[i].z;
        vertexPtr->bitangent.x = sourceMesh->mBitangents[i].x;
        vertexPtr->bitangent.y = sourceMesh->mBitangents[i].y;
        vertexPtr->bitangent.z = sourceMesh->mBitangents[i].z;

        if (sourceMesh->mTextureCoords[0]) {
            vertexPtr->texCoords.x = sourceMesh->mTextureCoords[0][i].x;
            vertexPtr->texCoords.y = sourceMesh->mTextureCoords[0][i].y;
        }
        vertexPtr++;
    }

    for (unsigned int i = 0; i < sourceMesh->mNumFaces; i++) {
        const aiFace &face = sourceMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            m_Indices.push_back(face.mIndices[j]);
    }
    m_VertexCount = sourceMesh->mNumVertices;
    m_AssimpMaterialIdx = sourceMesh->mMaterialIndex;
}


//void Mesh::SetMaterial(Material *material,
//                       const std::pair<uint32_t, uint32_t> &materialBinding,
//                       const std::unordered_map<Texture2D::Type, uint32_t> &textureIndices) {
//
//    const auto &materialLayout = material->VertexLayout();
//    if (m_VertexLayout.size() != materialLayout.size()) {
//        std::ostringstream msg;
//        msg << "[SetMaterial] Mismatch between the number of Mesh (" << m_VertexLayout.size()
//            << ") and ShaderProgram (" << materialLayout.size() << ") vertex attributes.";
//        throw std::runtime_error(msg.str().c_str());
//    }
//    size_t minAttribs = std::min(m_VertexLayout.size(), materialLayout.size());
//    for (size_t i = 0; i < minAttribs; i++) {
//        if (m_VertexLayout[i] != materialLayout[i]) {
//            std::ostringstream msg;
//            msg << "[SetMaterial] Mismatch between the size of Mesh (" << (int) m_VertexLayout[i]
//                << " bytes) and ShaderProgram (" << (int) materialLayout[i]
//                << " bytes) vertex attribute at location(" << i << ")";
//            throw std::runtime_error(msg.str().c_str());
//        }
//    }
//
//    if (m_Material) {
//        m_Material->OnDetach();
//    }
//
//    m_Material = material;
//    m_ObjectIdx = material->OnAttach();
//
//    MaterialUBO ubo{
//            glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
//            glm::vec4(1.0f),
//            glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
//            32.0f,
//            0,
//            0,
//            0
//    };
//
//    auto it = textureIndices.find(Texture2D::Type::DIFFUSE);
//    if (it != textureIndices.end()) ubo.diffuseTexIdx = it->second;
//
//    it = textureIndices.find(Texture2D::Type::SPECULAR);
//    if (it != textureIndices.end()) ubo.specularTexIdx = it->second;
//
//    m_Material->SetDynamicUniform(materialBinding.first, materialBinding.second, m_ObjectIdx, ubo);
//}
