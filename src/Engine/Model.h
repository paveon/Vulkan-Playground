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


class PerspectiveCamera;

class ModelAsset {
    std::vector<Material> m_Materials;
    std::vector<Mesh> m_Meshes;
    std::vector<std::unordered_map<Texture2D::Type, std::vector<const Texture2D *>>> m_Textures;

public:
    static auto LoadModel(const std::string &filepath) -> std::unique_ptr<ModelAsset>;

    static auto CreateCubeAsset() -> std::unique_ptr<ModelAsset>;

    ModelAsset() = default;

    ModelAsset(std::vector<Material> materials, std::vector<Mesh> meshes)
            : m_Materials(std::move(materials)), m_Meshes(std::move(meshes)) {
    }

    auto Meshes() -> std::vector<Mesh> & { return m_Meshes; }

    auto GetMaterial(size_t idx) -> Material & { return m_Materials[idx]; }

    auto GetMaterials() -> std::vector<Material> & { return m_Materials; }

    void StageMeshes() { for (auto &mesh : m_Meshes) mesh.StageData(); }

    auto Textures(uint32_t materialIdx) -> std::unordered_map<Texture2D::Type, std::vector<const Texture2D *>> & {
        return m_Textures[materialIdx];
    }

    auto Textures(uint32_t materialIdx, Texture2D::Type type) -> std::vector<const Texture2D *> {
        auto it = m_Textures[materialIdx].find(type);
        if (it != m_Textures[materialIdx].end())
            return it->second;
        else
            return {};
    }

    auto TextureVector(uint32_t materialIdx) -> std::vector<std::pair<Texture2D::Type, const Texture2D *>> {
        std::vector<std::pair<Texture2D::Type, const Texture2D *>> textures;
        for (const auto &it : m_Textures[materialIdx]) {
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

    void OnCreate() {
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

public:
    std::string m_Name;


    Entity(std::string name, std::unique_ptr<ModelAsset> &asset) : m_Name(std::move(name)) {
        OnCreate();

        for (auto &assetMesh : asset->Meshes()) {
            auto &meshMaterial = asset->GetMaterial(assetMesh.AssimpMaterialIdx());
            m_MeshRenderers.emplace_back(assetMesh.CreateInstance(m_InstanceID));
            m_MeshRenderers.back().SetMaterialInstance(meshMaterial.CreateInstance());
        }
    }

    explicit Entity(std::string name) : m_Name(std::move(name)) {
        OnCreate();
    };

    Entity(const Entity &other) = delete;

    auto operator=(const Entity &other) -> Entity & = delete;

    Entity(Entity &&other) noexcept = default;

    auto operator=(Entity &&other) noexcept -> Entity & = default;

    auto MeshRenderers() const -> const std::vector<MeshRenderer> & { return m_MeshRenderers; }

    auto AttachMesh(Mesh &mesh) -> MeshRenderer & {
        m_MeshRenderers.emplace_back(mesh.CreateInstance(m_InstanceID));
        return m_MeshRenderers.back();
    }

    void SetMeshMaterial(uint32_t meshIdx, Material &material) {
        m_MeshRenderers[meshIdx].SetMaterialInstance(material.CreateInstance());
    }

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


#endif //GAME_ENGINE_MODEL_H
