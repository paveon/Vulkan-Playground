#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtx/string_cast.hpp>

#include <Engine/Renderer/UniformBuffer.h>
#include "Model.h"
#include "Renderer/Mesh.h"
#include "Renderer/Camera.h"


std::unique_ptr<UniformBuffer> Entity::s_TransformsUB;
std::vector<glm::vec3> Entity::s_Positions;
std::vector<glm::vec3> Entity::s_Scales;
std::vector<glm::vec3> Entity::s_Rotations;
std::vector<glm::mat4> Entity::s_ModelMatrices;
std::vector<glm::mat4> Entity::s_NormalMatrices;
uint32_t Entity::s_InstanceCount = 0;


void Entity::AllocateTransformsUB(uint32_t entityCount) {
    s_TransformsUB = UniformBuffer::Create("Transforms UB", sizeof(TransformUBO), entityCount);
}


void Entity::UpdateTransformsUB(const PerspectiveCamera &camera) {
    std::vector<TransformUBO> ubos(s_InstanceCount);
    for (size_t i = 0; i < s_InstanceCount; i++) {
        ubos[i].model = s_ModelMatrices[i];
        ubos[i].view = camera.GetView();
        ubos[i].viewModel = camera.GetView() * s_ModelMatrices[i];
        ubos[i].mvp = camera.GetProjection() * ubos[i].viewModel;
        ubos[i].normalMatrix = camera.GetView() * s_NormalMatrices[i];
//        ubos[i].normalMatrix = glm::transpose(glm::inverse(camera.GetView() * m_ModelMatrices[i]));

//        ubos[i].normalMatrix = m_NormalMatrices[i] * camera.GetView();
//        std::fill(ubos.begin(), ubos.end(), TransformUBO{mvp, viewModel, normalMatrix});
    }
    s_TransformsUB->SetData(ubos.data(), s_InstanceCount, 0);
}


auto ModelAsset::LoadModel(const std::string &filepath) -> std::unique_ptr<ModelAsset> {

    static std::unordered_map<Texture2D::Type, aiTextureType> textureTypes{
            {Texture2D::Type::SPECULAR, aiTextureType_SPECULAR},
            {Texture2D::Type::DIFFUSE,  aiTextureType_DIFFUSE}
    };

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate |
                                                       aiProcess_FlipUVs |
                                                       aiProcess_JoinIdenticalVertices |
                                                       aiProcess_RemoveRedundantMaterials |
                                                       aiProcess_OptimizeMeshes |
                                                       aiProcess_OptimizeGraph);
    if (!scene || scene->mFlags & (unsigned) AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return {};
    }


    auto asset = std::make_unique<ModelAsset>();
    asset->m_Materials.reserve(scene->mNumMaterials);

    aiString tmp;
    for (size_t materialIdx = 0; materialIdx < scene->mNumMaterials; materialIdx++) {
        aiMaterial *sourceMaterial = scene->mMaterials[materialIdx];
        asset->m_Materials.emplace_back(sourceMaterial->GetName().C_Str());

        for (const auto &type : textureTypes) {
            auto textureCount = sourceMaterial->GetTextureCount(type.second);
            auto &textures = asset->m_Textures.emplace(type.first, std::vector<const Texture2D *>()).first->second;
            for (unsigned int i = 0; i < textureCount; i++) {
                sourceMaterial->GetTexture(type.second, i, &tmp);
                std::string textureName(tmp.C_Str());
                std::string path = std::string(BASE_DIR "/textures/") + textureName;
                textures.emplace_back(Texture2D::Create(path.c_str()));
//                asset->m_Materials[materialIdx].BindTextures(type.first, Texture2D::Create(path.c_str()));
            }
        }
    }

    asset->m_Meshes.reserve(scene->mNumMeshes);
    for (size_t i = 0; i < scene->mNumMeshes; i++) {
        const auto *sourceMesh = scene->mMeshes[i];
        asset->m_Meshes.emplace_back(sourceMesh, sourceMesh->mMaterialIndex);
    }

    return asset;
}


//void Model::SetMaterial(Material *material,
//                        std::pair<uint32_t, uint32_t> samplerBinding,
//                        std::pair<uint32_t, uint32_t> materialBinding) {
//    if (!m_Material || m_Material != material) {
//        m_Material = material;
//
//        std::vector<const Texture2D *> usedTextures;
//        for (const auto &modelMaterial : m_ModelMaterials) {
//            for (const auto &meshTexture : modelMaterial) {
//                const auto *texPtr = meshTexture.texture.get();
//                if (std::find(usedTextures.begin(), usedTextures.end(), texPtr) == usedTextures.end()) {
//                    usedTextures.emplace_back(texPtr);
//                }
//            }
//        }
//        auto textureIndices = m_Material->BindTextures(usedTextures, samplerBinding.first, samplerBinding.second);
//        std::vector<std::unordered_map<MeshTexture::Type, uint32_t>> partitionedIndices(m_ModelMaterials.size());
//        size_t offset = 0;
//        for (size_t matIdx = 0; matIdx < m_ModelMaterials.size(); matIdx++) {
//            for (const auto &meshTexture : m_ModelMaterials[matIdx]) {
//                partitionedIndices[matIdx].insert({meshTexture.type, textureIndices[offset]});
//                offset++;
//            }
//        }
//
//        for (const auto &mesh : m_Meshes) {
//            mesh->SetMaterial(m_Material, materialBinding, partitionedIndices[mesh->AssimpMaterialIdx()]);
//        }
//    }
//}





