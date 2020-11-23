#ifndef GAME_ENGINE_MODEL_H
#define GAME_ENGINE_MODEL_H

#include <cstdint>
#include <mathlib.h>
#include <memory>
#include <utility>
#include <vector>
#include <glm/ext/matrix_transform.hpp>
#include "Renderer/Material.h"
#include "Renderer/Mesh.h"

struct DynamicOffset {
    uint32_t set;
    uint32_t offset;
};

class Model;

class PerspectiveCamera;

class ModelAsset {
    std::vector<Material> m_Materials;
    std::vector<Mesh> m_Meshes;
    std::unordered_map<Texture2D::Type, std::vector<const Texture2D *>> m_Textures;

public:
    static auto LoadModel(const std::string &filepath) -> std::unique_ptr<ModelAsset>;

    ModelAsset() = default;

    ModelAsset(std::vector<Material> materials, std::vector<Mesh> meshes)
            : m_Materials(std::move(materials)), m_Meshes(std::move(meshes)) {
    }

    auto Meshes() -> std::vector<Mesh> & { return m_Meshes; }

    auto GetMaterial(size_t idx) -> Material & { return m_Materials[idx]; }

    auto GetMaterials() -> std::vector<Material> & { return m_Materials; }

    void StageMeshes() { for (auto &mesh : m_Meshes) mesh.StageData(); }

    auto Textures() -> std::unordered_map<Texture2D::Type, std::vector<const Texture2D *>> & { return m_Textures; }

    auto Textures(Texture2D::Type type) -> std::vector<const Texture2D *> {
        auto it = m_Textures.find(type);
        if (it != m_Textures.end())
            return it->second;
        else
            return {};
    }

    auto TextureVector() -> std::vector<std::pair<Texture2D::Type, const Texture2D *>> {
        std::vector<std::pair<Texture2D::Type, const Texture2D *>> textures;
        for (const auto &it : m_Textures) {
            auto type = it.first;
            std::transform(it.second.begin(), it.second.end(), std::back_inserter(textures),
                           [type](const Texture2D *tex) {
                               return std::make_pair(type, tex);
                           });
        }
        return textures;
    }
};


class Entity {
    static std::unique_ptr<UniformBuffer> s_TransformsUB;
    static std::vector<glm::vec3> s_Positions;
    static std::vector<glm::vec3> s_Scales;
    static std::vector<glm::vec3> s_Rotations;
    static std::vector<glm::mat4> s_ModelMatrices;
    static std::vector<glm::mat4> s_NormalMatrices;
    static uint32_t s_InstanceCount;

    std::vector<MeshRenderer> m_MeshRenderers;
    uint32_t m_InstanceID = 0;

    void RecalculateMatrices() const {
        const auto &pos = s_Positions[m_InstanceID];
        const auto &scale = s_Scales[m_InstanceID];
        const auto &rot = s_Rotations[m_InstanceID];
        auto rotX = glm::rotate(glm::scale(glm::mat4(1.0f), scale), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        auto rotXY = glm::rotate(rotX, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        auto rotXYZ = glm::rotate(rotXY, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
        s_ModelMatrices[m_InstanceID] = glm::translate(rotXYZ, pos);
        s_NormalMatrices[m_InstanceID] = glm::transpose(glm::inverse(s_ModelMatrices[m_InstanceID]));
    }

public:
    explicit Entity(std::unique_ptr<ModelAsset> &asset) {
        for (auto &assetMesh : asset->Meshes()) {
            auto &meshMaterial = asset->GetMaterial(assetMesh.AssimpMaterialIdx());
            m_MeshRenderers.emplace_back(assetMesh.GetInstance(m_InstanceID));
            m_MeshRenderers.back().SetMaterialInstance(meshMaterial.CreateInstance(BindingKey(4, 0)));
        }

        // TODO: will break when deleting instances, will fix later
        s_Positions.resize(s_InstanceCount + 1);
        s_Scales.resize(s_InstanceCount + 1);
        s_Rotations.resize(s_InstanceCount + 1);
        s_ModelMatrices.resize(s_InstanceCount + 1);
        s_NormalMatrices.resize(s_InstanceCount + 1);

        s_Positions[s_InstanceCount] = glm::vec3(0.0f);
        s_Scales[s_InstanceCount] = glm::vec3(1.0f);
        s_Rotations[s_InstanceCount] = glm::vec3(0.0f);
        s_ModelMatrices[s_InstanceCount] = glm::mat4(1.0f);
        s_NormalMatrices[s_InstanceCount] = glm::transpose(glm::inverse(glm::mat4(1.0f)));

        m_InstanceID = s_InstanceCount++;
    }

    Entity() = default;

    Entity(const Entity &other) = delete;

    auto operator=(const Entity &other) -> Entity & = delete;

    Entity(Entity &&other) noexcept = default;

    auto operator=(Entity &&other) noexcept -> Entity & = default;

    auto MeshRenderers() const -> const std::vector<MeshRenderer> & { return m_MeshRenderers; }

    void SetPosition(const glm::vec3 &position) const {
        s_Positions[m_InstanceID] = position;
        RecalculateMatrices();
    }

    void SetScale(const glm::vec3 &scale) const {
        s_Scales[m_InstanceID] = scale;
        RecalculateMatrices();
    }

    void SetRotationX(float angle) const {
        s_Rotations[m_InstanceID].x = angle;
        RecalculateMatrices();
    }

    void SetRotationY(float angle) const {
        s_Rotations[m_InstanceID].y = angle;
        RecalculateMatrices();
    }

    void SetRotationZ(float angle) const {
        s_Rotations[m_InstanceID].z = angle;
        RecalculateMatrices();
    }

    void SetRotation(const glm::vec3 &angles) const {
        s_Rotations[m_InstanceID] = angles;
        RecalculateMatrices();
    }

    auto GetPosition() const -> const glm::vec3 & { return s_Positions[m_InstanceID]; }

    auto GetScale() const -> const glm::vec3 & { return s_Scales[m_InstanceID]; }

    auto GetRotationX() const -> float { return s_Rotations[m_InstanceID].x; }

    auto GetRotationY() const -> float { return s_Rotations[m_InstanceID].y; }

    auto GetRotationZ() const -> float { return s_Rotations[m_InstanceID].z; }

    auto GetRotation() const -> const glm::vec3 & { return s_Rotations[m_InstanceID]; }

    static void AllocateTransformsUB(uint32_t entityCount);

    static void UpdateTransformsUB(const PerspectiveCamera &camera);

    static auto TransformsUB() -> const UniformBuffer * { return s_TransformsUB.get(); }
};


//class ModelInstance {
//    friend class Model;
//
//private:
//    Model *m_Model{};
//    uint32_t m_InstanceID{};
//
//    ModelInstance(Model *model, uint32_t id) : m_Model(model), m_InstanceID(id) {}
//
//    void Move(ModelInstance &other) {
//        m_Model = other.m_Model;
//        m_InstanceID = other.m_InstanceID;
//        other.m_Model = nullptr;
//        other.m_InstanceID = 0;
//    }
//
//public:
//    ModelInstance() = default;
//
//    ModelInstance(const ModelInstance &other) = delete;
//
//    auto operator=(const ModelInstance &other) -> ModelInstance & = delete;
//
//    ModelInstance(ModelInstance &&other) noexcept { Move(other); }
//
//    auto operator=(ModelInstance &&other) noexcept -> ModelInstance & {
//        Move(other);
//        return *this;
//    };
//
//    auto GetModel() const -> Model * { return m_Model; }
//
//    void SetPosition(const glm::vec3 &position);
//
//    void SetScale(const glm::vec3 &scale);
//
//    void SetRotationX(float angle);
//
//    void SetRotationY(float angle);
//
//    void SetRotationZ(float angle);
//
//    void SetRotation(const glm::vec3 &angles);
//
//    auto GetScale() const -> const glm::vec3 &;
//
//    auto GetPosition() const -> const glm::vec3 &;
//
//    auto GetRotationX() -> float;
//
//    auto GetRotationY() -> float;
//
//    auto GetRotationZ() -> float;
//
//    auto GetRotation() -> const glm::vec3 &;
//};


//class Model {
//    friend class ModelInstance;
//
//private:
//    std::vector<glm::vec3> m_InstancePositions;
//    std::vector<glm::vec3> m_InstanceScales;
//    std::vector<glm::vec3> m_InstanceRotations;
//    std::vector<glm::mat4> m_InstanceModelMatrices;
//    std::vector<glm::mat4> m_InstanceNormalMatrices;
//    uint32_t m_InstanceCount = 0;
//
//    std::vector<DynamicOffset> m_DynamicOffsets;
//    std::vector<std::shared_ptr<Mesh>> m_Meshes;
//    std::vector<std::vector<MeshTexture>> m_ModelMaterials;
//    std::string m_Directory;
//    Material *m_Material = nullptr;
//
//    void RecalculateMatrices(uint32_t id) {
//        const auto &pos = m_InstancePositions[id];
//        const auto &scale = m_InstanceScales[id];
//        const auto &rot = m_InstanceRotations[id];
//        auto rotX = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
//        auto rotXY = glm::rotate(rotX, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
//        auto rotXYZ = glm::rotate(rotXY, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
//        m_InstanceModelMatrices[id] = glm::scale(glm::translate(rotXYZ, pos), scale);
//        m_InstanceNormalMatrices[id] = glm::transpose(glm::inverse(m_InstanceModelMatrices[id]));
//    }
//
//    void SetPosition(uint32_t id, const glm::vec3 &position) {
//        m_InstancePositions[id] = position;
//        RecalculateMatrices(id);
//    }
//
//    void SetScale(uint32_t id, const glm::vec3 &scale) {
//        m_InstanceScales[id] = scale;
//        RecalculateMatrices(id);
//    }
//
//    void SetRotationX(uint32_t id, float angle) {
//        m_InstanceRotations[id].x = angle;
//        RecalculateMatrices(id);
//    }
//
//    void SetRotationY(uint32_t id, float angle) {
//        m_InstanceRotations[id].y = angle;
//        RecalculateMatrices(id);
//    }
//
//    void SetRotationZ(uint32_t id, float angle) {
//        m_InstanceRotations[id].z = angle;
//        RecalculateMatrices(id);
//    }
//
//    void SetRotation(uint32_t id, const glm::vec3 &angles) {
//        m_InstanceRotations[id] = angles;
//        RecalculateMatrices(id);
//    }
//
//    auto GetPosition(uint32_t id) const -> const glm::vec3 & { return m_InstancePositions[id]; }
//
//    auto GetScale(uint32_t id) const -> const glm::vec3 & { return m_InstanceScales[id]; }
//
//    auto GetRotationX(uint32_t id) const -> float { return m_InstanceRotations[id].x; }
//
//    auto GetRotationY(uint32_t id) const -> float { return m_InstanceRotations[id].y; }
//
//    auto GetRotationZ(uint32_t id) const -> float { return m_InstanceRotations[id].z; }
//
//    auto GetRotation(uint32_t id) const -> const glm::vec3 & { return m_InstanceRotations[id]; }
//
//public:
//    Model() = default;
//
//    void AddDynamicOffset(const DynamicOffset &offset) { m_DynamicOffsets.emplace_back(offset); }
//
//    auto GetMeshes() const -> const auto & { return m_Meshes; }
//
//    void StageMeshData() const { for (const auto &mesh : m_Meshes) mesh->StageData(); }
//
//    auto GetDynamicOffsets() const -> const std::vector<DynamicOffset> & { return m_DynamicOffsets; }
//
//    void SetMaterial(Material *material, std::pair<uint32_t, uint32_t> samplerBinding,
//                     std::pair<uint32_t, uint32_t> materialBinding);
//
//    auto GetMaterial() const -> Material * { return m_Material; }
//
//    template<typename T>
//    void SetUniform(uint32_t set, uint32_t binding, const T &value) const {
////        m_Material->SetDynamicUniform(set, binding, m_MaterialResourceIndex, value);
//    }
//
//    auto CreateInstance() -> ModelInstance {
//        // TODO: will break when deleting instances, will fix later
//        m_InstancePositions.resize(m_InstanceCount + 1);
//        m_InstanceScales.resize(m_InstanceCount + 1);
//        m_InstanceRotations.resize(m_InstanceCount + 1);
//        m_InstanceModelMatrices.resize(m_InstanceCount + 1);
//        m_InstanceNormalMatrices.resize(m_InstanceCount + 1);
//
//        m_InstancePositions[m_InstanceCount] = glm::vec3(0.0f);
//        m_InstanceScales[m_InstanceCount] = glm::vec3(1.0f);
//        m_InstanceRotations[m_InstanceCount] = glm::vec3(0.0f);
//        m_InstanceModelMatrices[m_InstanceCount] = glm::mat4(1.0f);
//        m_InstanceNormalMatrices[m_InstanceCount] = glm::transpose(glm::inverse(glm::mat4(1.0f)));
//        return ModelInstance(this, m_InstanceCount++);
//    }
//};


//inline void ModelInstance::SetPosition(const glm::vec3 &position) { m_Model->SetPosition(m_InstanceID, position); }
//
//inline auto ModelInstance::GetPosition() const -> const glm::vec3 & { return m_Model->GetPosition(m_InstanceID); }
//
//inline void ModelInstance::SetScale(const glm::vec3 &scale) { m_Model->SetScale(m_InstanceID, scale); }
//
//inline auto ModelInstance::GetScale() const -> const glm::vec3 & { return m_Model->GetScale(m_InstanceID); }
//
//inline void ModelInstance::SetRotationX(float angle) { m_Model->SetRotationX(m_InstanceID, angle); }
//
//inline void ModelInstance::SetRotationY(float angle) { m_Model->SetRotationY(m_InstanceID, angle); }
//
//inline void ModelInstance::SetRotationZ(float angle) { m_Model->SetRotationZ(m_InstanceID, angle); }
//
//inline void ModelInstance::SetRotation(const glm::vec3 &angles) { m_Model->SetRotation(m_InstanceID, angles); }
//
//inline auto ModelInstance::GetRotationX() -> float { return m_Model->GetRotationX(m_InstanceID); }
//
//inline auto ModelInstance::GetRotationY() -> float { return m_Model->GetRotationY(m_InstanceID); }
//
//inline auto ModelInstance::GetRotationZ() -> float { return m_Model->GetRotationZ(m_InstanceID); }
//
//inline auto ModelInstance::GetRotation() -> const glm::vec3 & { return m_Model->GetRotation(m_InstanceID); }


#endif //GAME_ENGINE_MODEL_H
