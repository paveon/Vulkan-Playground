#ifndef GAME_ENGINE_MODEL_H
#define GAME_ENGINE_MODEL_H

#include <cstdint>
#include <mathlib.h>
#include <memory>
#include <vector>
#include <glm/ext/matrix_transform.hpp>
#include "Renderer/Material.h"
#include "Renderer/Mesh.h"

struct DynamicOffset {
    uint32_t set;
    uint32_t offset;
};


class Mesh;

class Material;

class aiNode;

class aiScene;

class Model;

class PerspectiveCamera;

class ModelInstance {
    friend class Model;

private:
    Model *m_Model{};
    uint32_t m_InstanceID{};

    ModelInstance(Model *model, uint32_t id);

    void Move(ModelInstance &other) {
        m_Model = other.m_Model;
        m_InstanceID = other.m_InstanceID;
    }

public:
    ModelInstance() = default;

    ModelInstance(const ModelInstance &other) = delete;

    auto operator=(const ModelInstance &other) -> ModelInstance & = delete;

    ModelInstance(ModelInstance &&other) noexcept { Move(other); }

    auto operator=(ModelInstance &&other) noexcept -> ModelInstance & {
        if (this == &other) return *this;
        Move(other);
        return *this;
    };

    auto GetModel() const -> Model * { return m_Model; }

    void SetPosition(const glm::vec3 &position);

    void SetScale(const glm::vec3 &scale);

    void SetRotationX(float angle);

    void SetRotationY(float angle);

    void SetRotationZ(float angle);

    void SetRotation(const glm::vec3 &angles);

    auto GetScale() const -> const glm::vec3 &;

    auto GetPosition() const -> const glm::vec3 &;

    auto GetRotationX() -> float;

    auto GetRotationY() -> float;

    auto GetRotationZ() -> float;

    auto GetRotation() -> const glm::vec3 &;
};


class Model {
    friend class ModelInstance;

private:
    std::vector<glm::vec3> m_InstancePositions;
    std::vector<glm::vec3> m_InstanceScales;
    std::vector<glm::vec3> m_InstanceRotations;
    std::vector<glm::mat4> m_InstanceModelMatrices;
    std::vector<glm::mat4> m_InstanceNormalMatrices;
    uint32_t m_InstanceCount = 0;

//    glm::mat4 m_Model = glm::mat4(1.0f);
//    glm::mat4 m_NormalMatrix = glm::mat4(1.0f);
//    glm::vec3 m_Position = glm::vec3(0.0f);
//    glm::vec3 m_Scale = glm::vec3(1.0f);
//    glm::vec3 m_Color = glm::vec3(1.0f);
    std::vector<DynamicOffset> m_DynamicOffsets;
    std::vector<std::shared_ptr<Mesh>> m_Meshes;
    std::vector<std::vector<MeshTexture>> m_ModelMaterials;
    std::string m_Directory;
    Material *m_Material = nullptr;

    void RecalculateMatrices(uint32_t id) {
        const auto& pos = m_InstancePositions[id];
        const auto& scale = m_InstanceScales[id];
        const auto& rot = m_InstanceRotations[id];
        auto rotX = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        auto rotXY = glm::rotate(rotX, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        auto rotXYZ = glm::rotate(rotXY, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
        m_InstanceModelMatrices[id] = glm::scale(glm::translate(rotXYZ, pos), scale);
        m_InstanceNormalMatrices[id] = glm::transpose(glm::inverse(m_InstanceModelMatrices[id]));
    }

    void loadModel(const std::string &filepath, const std::vector<MeshTexture::Type> &textureTypes);

    void processNode(const aiNode *node, const aiScene *scene);

    void SetPosition(uint32_t id, const glm::vec3 &position) {
        m_InstancePositions[id] = position;
        RecalculateMatrices(id);
    }

    void SetScale(uint32_t id, const glm::vec3 &scale) {
        m_InstanceScales[id] = scale;
        RecalculateMatrices(id);
    }

    void SetRotationX(uint32_t id, float angle) {
        m_InstanceRotations[id].x = angle;
        RecalculateMatrices(id);
    }

    void SetRotationY(uint32_t id, float angle) {
        m_InstanceRotations[id].y = angle;
        RecalculateMatrices(id);
    }

    void SetRotationZ(uint32_t id, float angle) {
        m_InstanceRotations[id].z = angle;
        RecalculateMatrices(id);
    }

    void SetRotation(uint32_t id, const glm::vec3 &angles) {
        m_InstanceRotations[id] = angles;
        RecalculateMatrices(id);
    }

    auto GetPosition(uint32_t id) const -> const glm::vec3 & { return m_InstancePositions[id]; }

    auto GetScale(uint32_t id) const -> const glm::vec3 & { return m_InstanceScales[id]; }

    auto GetRotationX(uint32_t id) const -> float { return m_InstanceRotations[id].x; }

    auto GetRotationY(uint32_t id) const -> float { return m_InstanceRotations[id].y; }

    auto GetRotationZ(uint32_t id) const -> float { return m_InstanceRotations[id].z; }

    auto GetRotation(uint32_t id) const -> const glm::vec3 & { return m_InstanceRotations[id]; }

public:
    Model() = default;

    explicit Model(const std::string &filepath);

    static auto Cube() -> Model;

//    void SetPosition(const glm::vec3 &position) {
//        m_Position = position;
//        RecalculateMatrices(id);
//    }
//
//    void SetScale(const glm::vec3 &scale) {
//        m_Scale = scale;
//        RecalculateMatrices();
//    }

    void SetTransformUniformData(const PerspectiveCamera &camera, uint32_t set, uint32_t binding) const;

//    void SetColor(const glm::vec3 &color) { m_Color = color; }
//
//    auto GetColor() const -> const auto & { return m_Color; }
//
//    auto GetModelMatrix() const -> glm::mat4 { return m_Model; }
//
//    auto GetNormalMatrix() const -> glm::mat4 { return m_NormalMatrix; }

    void AddDynamicOffset(const DynamicOffset &offset) { m_DynamicOffsets.emplace_back(offset); }

    void AttachMesh(const std::shared_ptr<Mesh> &mesh) {
//        m_Meshes.emplace_back(mesh);
//        if (m_Material) mesh->SetMaterial(m_Material);
    }

    auto GetMeshes() const -> const auto & { return m_Meshes; }

    void StageMeshData() const { for (const auto &mesh : m_Meshes) mesh->StageData(); }

    auto GetDynamicOffsets() const -> const std::vector<DynamicOffset> & { return m_DynamicOffsets; }

    void SetMaterial(Material *material, std::pair<uint32_t, uint32_t> samplerBinding,
                     std::pair<uint32_t, uint32_t> materialBinding);

    auto GetMaterial() const -> Material * { return m_Material; }

    template<typename T>
    void SetUniform(uint32_t set, uint32_t binding, const T &value) const {
//        m_Material->SetDynamicUniform(set, binding, m_MaterialResourceIndex, value);
    }

    auto CreateInstance() -> ModelInstance {
        // TODO: will break when deleting instances, will fix later
        m_InstancePositions.resize(m_InstanceCount + 1);
        m_InstanceScales.resize(m_InstanceCount + 1);
        m_InstanceRotations.resize(m_InstanceCount + 1);
        m_InstanceModelMatrices.resize(m_InstanceCount + 1);
        m_InstanceNormalMatrices.resize(m_InstanceCount + 1);

        m_InstancePositions[m_InstanceCount] = glm::vec3(0.0f);
        m_InstanceScales[m_InstanceCount] = glm::vec3(1.0f);
        m_InstanceRotations[m_InstanceCount] = glm::vec3(0.0f);
        m_InstanceModelMatrices[m_InstanceCount] = glm::mat4(1.0f);
        m_InstanceNormalMatrices[m_InstanceCount] = glm::transpose(glm::inverse(glm::mat4(1.0f)));
        return ModelInstance(this, m_InstanceCount++);
    }
};


inline void ModelInstance::SetPosition(const glm::vec3 &position) { m_Model->SetPosition(m_InstanceID, position); }

inline auto ModelInstance::GetPosition() const -> const glm::vec3 & { return m_Model->GetPosition(m_InstanceID); }

inline void ModelInstance::SetScale(const glm::vec3 &scale) { m_Model->SetScale(m_InstanceID, scale); }

inline auto ModelInstance::GetScale() const -> const glm::vec3 & { return m_Model->GetScale(m_InstanceID); }

inline void ModelInstance::SetRotationX(float angle) { m_Model->SetRotationX(m_InstanceID, angle); }

inline void ModelInstance::SetRotationY(float angle) { m_Model->SetRotationY(m_InstanceID, angle); }

inline void ModelInstance::SetRotationZ(float angle) { m_Model->SetRotationZ(m_InstanceID, angle); }

inline void ModelInstance::SetRotation(const glm::vec3 &angles) { m_Model->SetRotation(m_InstanceID, angles); }

inline auto ModelInstance::GetRotationX() -> float { return m_Model->GetRotationX(m_InstanceID); }

inline auto ModelInstance::GetRotationY() -> float { return m_Model->GetRotationY(m_InstanceID); }

inline auto ModelInstance::GetRotationZ() -> float { return m_Model->GetRotationZ(m_InstanceID); }

inline auto ModelInstance::GetRotation() -> const glm::vec3 & { return m_Model->GetRotation(m_InstanceID); }

inline ModelInstance::ModelInstance(Model *model, uint32_t id) : m_Model(model), m_InstanceID(id) {}


#endif //GAME_ENGINE_MODEL_H
