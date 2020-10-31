#ifndef GAME_ENGINE_MODEL_H
#define GAME_ENGINE_MODEL_H

#include <cstdint>
#include <mathlib.h>
#include <memory>
#include <vector>

#include <Engine/Renderer/Mesh.h>


class Model {
private:
    std::shared_ptr<Mesh> m_Mesh;
//    std::vector<Vertex> m_Vertices;
//    std::vector<VertexIndex> m_Indices;
//    std::unique_ptr<Texture2D> m_Texture;

public:
    Model();

    void AttachMesh(const std::shared_ptr<Mesh>& mesh) { m_Mesh = mesh; }

    auto GetMesh() -> Mesh& { return *m_Mesh; }
};


#endif //GAME_ENGINE_MODEL_H
