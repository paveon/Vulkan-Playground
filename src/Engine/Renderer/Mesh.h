#ifndef GAME_ENGINE_MESH_H
#define GAME_ENGINE_MESH_H

#include <memory>
#include "Material.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"


class Mesh {
private:
    std::shared_ptr<Material> m_Material{};
    std::unique_ptr<VertexBuffer> m_VertexBuffer{};
    std::unique_ptr<IndexBuffer> m_IndexBuffer{};

public:
    Mesh(std::unique_ptr<VertexBuffer> vertexBuffer, std::unique_ptr<IndexBuffer> indexBuffer) :
            m_VertexBuffer(std::move(vertexBuffer)), m_IndexBuffer(std::move(indexBuffer)) {}

    auto VertexBuffer() const -> const VertexBuffer & { return *m_VertexBuffer; }

    auto IndexBuffer() const -> const IndexBuffer & { return *m_IndexBuffer; }

    void SetMaterial(const std::shared_ptr<Material>& material) { m_Material = material; }

    auto GetMaterial() -> Material& { return *m_Material; }
};


#endif //GAME_ENGINE_MESH_H
