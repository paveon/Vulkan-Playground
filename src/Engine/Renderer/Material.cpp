#include "Material.h"

#include <utility>
#include <iostream>
#include "Renderer.h"


Material::Material(std::string name) : m_Name(std::move(name)) {}


Material::Material(std::string name, std::shared_ptr<ShaderPipeline> shaderPipeline) : m_Name(std::move(name)) {
    SetShaderPipeline(std::move(shaderPipeline));
}


void Material::SetShaderPipeline(std::shared_ptr<ShaderPipeline> pipeline) {
    if (m_ShaderPipeline == pipeline) {
        return;
    }

    if (m_ShaderPipeline) {
        /// TODO: pass texture indices so we can free texture bindings in the shader
        m_ShaderPipeline->OnDetach(m_MaterialID);
    }
    m_ShaderPipeline = std::move(pipeline);
    m_MaterialID = m_ShaderPipeline->OnAttach(this);

    m_VertexLayout = m_ShaderPipeline->VertexLayout();
    m_SharedUniformData.clear();
    m_UniformData.clear();
    for (const auto &[key, shaderUniform] : m_ShaderPipeline->ShaderUniforms()) {
        m_SharedUniformData.emplace(key, Uniform{
                std::vector<uint8_t>(shaderUniform.size),
                shaderUniform.size,
                shaderUniform.perObject
        });

        if (shaderUniform.perObject) {
            m_UniformData.emplace(key, Uniform{
                    std::vector<uint8_t>(shaderUniform.size * m_InstanceCount),
                    shaderUniform.size,
                    shaderUniform.perObject
            });
        }
    }
}

//void Material::AllocateResources(uint32_t objectCount) {
//    m_ShaderPipeline->AllocateResources(objectCount);
//}

void Material::UpdateUniforms() {
    for (const auto&[key, sharedUniform] : m_SharedUniformData) {
        if (sharedUniform.perObject) {
            if (m_InstanceCount > 0) {
                auto &uniform = m_UniformData.find(key)->second;
                m_ShaderPipeline->SetUniformData(m_MaterialID, key, uniform.data.data(), m_InstanceCount);
            }
        } else {
            m_ShaderPipeline->SetUniformData(m_MaterialID, key, sharedUniform.data.data(), 1);
        }
    }


//    for (auto&[key, uniform] : m_UniformData) {
////        size_t requiredDataSize = uniform.perObject ? uniform.objectSize * m_InstanceCount : uniform.objectSize;
//        uint32_t objectCount = uniform.perObject ? m_InstanceCount : 1;
////        if (uniform.data.size() < requiredDataSize) {
////            uniform.data.resize(requiredDataSize);
////        }
//        m_ShaderPipeline->SetUniformData(m_MaterialID, key, uniform.data.data(), objectCount);
//    }
}

//auto Material::BindTextures(const std::vector<std::pair<Texture2D::Type, const Texture2D *>> &textures,
//                            BindingKey bindingKey) -> std::vector<uint32_t> {
//
//    if (!m_ShaderPipeline) {
//        std::cout << "[(" << m_Name << ")->BindTextures] Missing ShaderProgram, ignoring texture bind" << std::endl;
//        return {};
//    }
//
//    std::vector<BoundTexture2D> metadata(textures.size());
//    std::vector<const Texture2D *> tmp(textures.size());
//    for (size_t i = 0; i < textures.size(); i++) {
//        metadata[i].texture = textures[i].second;
//        tmp[i] = textures[i].second;
//    }
//
//    auto indices = m_ShaderPipeline->BindTextures(tmp, bindingKey);
//    for (size_t i = 0; i < textures.size(); i++) {
//        metadata[i].samplerIdx = indices[i];
//    }
//
//    for (size_t i = 0; i < textures.size(); i++) {
//        const auto&[texType, texture] = textures[i];
//        TextureKey key(bindingKey, texType);
//        auto it = m_BoundTextures2D.find(key);
//        if (it == m_BoundTextures2D.end()) {
//            it = m_BoundTextures2D.emplace(key, std::vector<BoundTexture2D>()).first;
//        }
//        it->second.push_back(metadata[i]);
//    }
//
//    return indices;
//}


auto Material::BindTextures(const std::unordered_map<Texture2D::Type, const Texture2D *> &textures,
                            BindingKey bindingKey) -> std::unordered_map<Texture2D::Type, uint32_t> {

    if (!m_ShaderPipeline) {
        std::cout << "[(" << m_Name << ")->BindTextures2D] Missing ShaderProgram, ignoring texture bind" << std::endl;
        return {};
    }

    std::vector<const Texture2D *> textureVector(textures.size());
    std::transform(textures.begin(), textures.end(), textureVector.begin(), [](auto x) { return x.second; });

    auto indices = m_ShaderPipeline->BindTextures2D(textureVector, bindingKey);
    std::unordered_map<Texture2D::Type, uint32_t> mapping;
    size_t idx = 0;
    for (const auto&[type, texture] : textures) {
        mapping.emplace(type, indices[idx]);

        TextureKey key(bindingKey, type);
        auto it = m_BoundTextures2D.find(key);
        if (it == m_BoundTextures2D.end()) {
            it = m_BoundTextures2D.emplace(key, std::vector<BoundTexture2D>()).first;
        }
        it->second.push_back(BoundTexture2D{texture, indices[idx]});

        idx++;
    }

    return mapping;
}


auto Material::BindCubemaps(const std::unordered_map<TextureCubemap::Type, const TextureCubemap *>& textures,
                           BindingKey bindingKey) -> std::unordered_map<TextureCubemap::Type, uint32_t> {
    if (!m_ShaderPipeline) {
        std::cout << "[(" << m_Name << ")->BindCubemaps] Missing ShaderProgram, ignoring texture bind" << std::endl;
        return {};
    }

    std::vector<const TextureCubemap *> textureVector(textures.size());
    std::transform(textures.begin(), textures.end(), textureVector.begin(), [](auto x) { return x.second; });

    auto indices = m_ShaderPipeline->BindCubemaps(textureVector, bindingKey);
    std::unordered_map<TextureCubemap::Type, uint32_t> mapping;
    size_t idx = 0;
    for (const auto&[type, texture] : textures) {
        mapping.emplace(type, indices[idx]);

        TextureKey key(bindingKey, type);
        auto it = m_BoundCubemaps.find(key);
        if (it == m_BoundCubemaps.end()) {
            it = m_BoundCubemaps.emplace(key, std::vector<BoundCubemap>()).first;
        }
        it->second.push_back(BoundCubemap{texture, indices[idx]});

        idx++;
    }

    return mapping;

//    auto indices = m_ShaderPipeline->BindCubemaps({texture}, bindingKey);
//    BoundCubemap metadata{
//            texture,
//            0
//    };
//
//    auto it = m_BoundCubemaps.find(bindingKey);
//    if (it == m_BoundCubemaps.end()) {
//        it = m_BoundCubemaps.emplace(bindingKey, std::vector<BoundCubemap>()).first;
//    }
//    it->second.push_back(metadata);
//    return metadata.samplerIdx;
}

auto Material::CreateInstance() -> MaterialInstance {
    auto newInstanceID = m_InstanceCount++;
    for (auto&[key, uniform] : m_UniformData) {
        uniform.data.resize(uniform.objectSize * m_InstanceCount);

        auto sharedDataIt = m_SharedUniformData.find(key);
        if (sharedDataIt == m_SharedUniformData.end()) {
            std::cout << "[(" << m_Name << ")->CreateInstance] Missing shared material state data for uniform"
                      << " structure at binding {" << key.Set() << ";" << key.Binding() << "}" << std::endl;
        } else {
            auto &sharedUniform = sharedDataIt->second;
            size_t instanceOffset = uniform.objectSize * newInstanceID;
            std::memcpy(&uniform.data[instanceOffset], sharedUniform.data.data(), uniform.objectSize);
//            SetInstanceUniform(key, newInstanceID, sharedUniform.data.data());
        }
    }
//    m_MaterialUBOs.resize(m_InstanceCount);
//
//    /// TODO: this is not generic yet, will fix later and add proper support for
//    /// material instances with different textures and uniform buffers
//    MaterialUBO ubo{
//            glm::vec4(0.0f),
//            glm::vec4(0.0f),
//            glm::vec4(0.0f),
//            32,
//            -1, -1, -1
//    };
//    for (const auto&[texKey, textures] : m_BoundTextures) {
//        switch (texKey.TextureType()) {
//            case Texture2D::Type::DIFFUSE:
//                if (!textures.empty()) ubo.diffuseTexIdx = textures[0].samplerIdx;
//                break;
//            case Texture2D::Type::SPECULAR:
//                if (!textures.empty()) ubo.specularTexIdx = textures[0].samplerIdx;
//                break;
//        }
//    }
//
//    m_MaterialUBOs[newInstanceID] = ubo;
//    SetDynamicUniforms<MaterialUBO>(m_MaterialBinding, newInstanceID, {ubo});
    return MaterialInstance(this, newInstanceID);
}
