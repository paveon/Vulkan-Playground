#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Model.h"
#include "Renderer/Mesh.h"
#include "Renderer/Camera.h"


void Model::SetMaterial(Material *material,
                        std::pair<uint32_t, uint32_t> samplerBinding,
                        std::pair<uint32_t, uint32_t> materialBinding) {
    if (!m_Material || m_Material != material) {
        m_Material = material;

        std::vector<const Texture2D *> usedTextures;
        for (const auto &modelMaterial : m_ModelMaterials) {
            for (const auto &meshTexture : modelMaterial) {
                const auto *texPtr = meshTexture.texture.get();
                if (std::find(usedTextures.begin(), usedTextures.end(), texPtr) == usedTextures.end()) {
                    usedTextures.emplace_back(texPtr);
                }
            }
        }
        auto textureIndices = m_Material->BindTextures(usedTextures, samplerBinding.first, samplerBinding.second);
        std::vector<std::unordered_map<MeshTexture::Type, uint32_t>> partitionedIndices(m_ModelMaterials.size());
        size_t offset = 0;
        for (size_t matIdx = 0; matIdx < m_ModelMaterials.size(); matIdx++) {
            for (const auto &meshTexture : m_ModelMaterials[matIdx]) {
                partitionedIndices[matIdx].insert({meshTexture.type, textureIndices[offset]});
                offset++;
            }
        }

        for (const auto &mesh : m_Meshes) {
            mesh->SetMaterial(m_Material, materialBinding, partitionedIndices[mesh->AssimpMaterialIdx()]);
        }
    }
}


Model::Model(const std::string &filepath) : m_Directory(filepath.substr(0, filepath.find_last_of('/'))) {
    loadModel(filepath, {MeshTexture::Type::SPECULAR, MeshTexture::Type::DIFFUSE});
}


void Model::loadModel(const std::string &filepath, const std::vector<MeshTexture::Type> &textureTypes) {
    static std::unordered_map<std::string, std::shared_ptr<Texture2D>> loadedTextures;

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate |
                                                       aiProcess_FlipUVs |
                                                       aiProcess_JoinIdenticalVertices |
                                                       aiProcess_RemoveRedundantMaterials |
                                                       aiProcess_OptimizeMeshes |
                                                       aiProcess_OptimizeGraph);
    if (!scene || scene->mFlags & (unsigned) AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    std::vector<std::pair<MeshTexture::Type, aiTextureType>> typePairs;
    for (const auto &type : textureTypes) {
        switch (type) {
            case MeshTexture::Type::DIFFUSE:
                typePairs.emplace_back(type, aiTextureType_DIFFUSE);
                break;
            case MeshTexture::Type::SPECULAR:
                typePairs.emplace_back(type, aiTextureType_SPECULAR);
                break;
            default:
                assert(false);
        }
    }

    m_ModelMaterials.resize(scene->mNumMaterials);
    for (size_t materialIdx = 0; materialIdx < m_ModelMaterials.size(); materialIdx++) {
        aiMaterial *material = scene->mMaterials[materialIdx];

        for (const auto &type : typePairs) {
            auto textureCount = material->GetTextureCount(type.second);
            for (unsigned int i = 0; i < textureCount; i++) {
                aiString str;
                material->GetTexture(type.second, i, &str);
                std::string textureName(str.C_Str());
                auto it = loadedTextures.find(textureName);
                if (it == loadedTextures.end()) {
                    std::string path = std::string(BASE_DIR "/textures/") + str.C_Str();
                    std::shared_ptr<Texture2D> tex = Texture2D::Create(path.c_str());
                    tex->Upload();
                    it = loadedTextures.insert({textureName, tex}).first;
                }
                m_ModelMaterials[materialIdx].emplace_back(MeshTexture{it->second, type.first});
            }
        }
    }

    processNode(scene->mRootNode, scene);
}


void Model::processNode(const aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        m_Meshes.push_back(Mesh::Create(mesh));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void Model::SetTransformUniformData(const PerspectiveCamera &camera, uint32_t set, uint32_t binding) const {
    std::vector<TransformUBO> ubos(m_Meshes.size());
//    size_t offset = 0;
    for (size_t i = 0; i < m_InstanceCount; i++) {
        glm::mat4 viewModel = camera.GetView() * m_InstanceModelMatrices[i];
        glm::mat4 mvp = camera.GetProjection() * viewModel;
        glm::mat4 normalMatrix = m_InstanceNormalMatrices[i] * camera.GetView();
        std::fill(ubos.begin(), ubos.end(), TransformUBO{mvp, viewModel, normalMatrix});
        m_Material->SetDynamicUniforms(set, binding, m_Meshes[0]->GetMaterialObjectIdx(), ubos);
    }
}

auto Model::Cube() -> Model {
    Model cube;
    cube.AttachMesh(Mesh::Cube());
    return Model();
}

