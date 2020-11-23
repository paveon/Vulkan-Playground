#include "Material.h"

#include <utility>
#include <iostream>
#include "Renderer.h"


Material::Material(std::string name) : m_Name(std::move(name)) {

}


Material::Material(std::string name, std::shared_ptr<ShaderPipeline> shaderPipeline, BindingKey materialSlot) : m_Name(std::move(name)) {
    SetShaderPipeline(std::move(shaderPipeline), materialSlot);
}


void Material::SetShaderPipeline(std::shared_ptr<ShaderPipeline> pipeline, BindingKey materialSlot) {
    if (m_ShaderPipeline == pipeline) {
        m_MaterialBinding = materialSlot;
        return;
    }

    if (m_ShaderPipeline) {
        /// TODO: pass texture indices so we can free texture bindings in the shader
        m_ShaderPipeline->OnDetach(m_MaterialID);
    }
    m_ShaderPipeline = std::move(pipeline);
    m_MaterialID = m_ShaderPipeline->OnAttach(this);

    m_VertexLayout = m_ShaderPipeline->VertexLayout();
    for (const auto &[key, shaderUniform] : m_ShaderPipeline->ShaderUniforms()) {
        Uniform uniform{{}, shaderUniform.size, shaderUniform.perObject};
        if (!shaderUniform.perObject) {
            uniform.data.resize(shaderUniform.size);
        }
        m_UniformData.emplace(key, uniform);
    }

    m_MaterialBinding = materialSlot;
}

//void Material::AllocateResources(uint32_t objectCount) {
//    m_ShaderPipeline->AllocateResources(objectCount);
//}

void Material::UpdateUniforms() {
    for (auto&[key, uniform] : m_UniformData) {
        size_t requiredDataSize = uniform.dynamic ? uniform.objectSize * m_InstanceCount : uniform.objectSize;
        uint32_t objectCount = uniform.dynamic ? m_InstanceCount : 1;
        if (uniform.data.size() < requiredDataSize) {
            uniform.data.resize(requiredDataSize);
        }
        m_ShaderPipeline->SetUniformData(m_MaterialID, key, uniform.data.data(), objectCount);
    }
}

void Material::BindTextures(const std::vector<std::pair<Texture2D::Type, const Texture2D *>> &textures,
                            BindingKey bindingKey) {

    if (!m_ShaderPipeline) {
        std::cout << "[(" << m_Name << ")->BindTextures] Missing ShaderProgram, ignoring texture bind" << std::endl;
        return;
    }

    std::vector<BoundTexture> metadata(textures.size());
    std::vector<const Texture2D *> tmp(textures.size());
    for (size_t i = 0; i < textures.size(); i++) {
        metadata[i].texture = textures[i].second;
        tmp[i] = textures[i].second;
    }

    auto indices = m_ShaderPipeline->BindTextures(tmp, bindingKey);
    for (size_t i = 0; i < textures.size(); i++) {
        metadata[i].samplerIdx = indices[i];
    }
//        auto textureCount = m_BoundTextures.size();
//        m_BoundTextures.reserve(textureCount + textures.size());
//        m_BoundTextures.insert(m_BoundTextures.end(), textures.begin(), textures.end());
//        m_ShaderPipeline->BindTextures(set, binding, m_BoundTextures);
//        std::vector<uint32_t> indices(textures.size());
//        std::iota(indices.begin(), indices.end(), textureCount);

    for (size_t i = 0; i < textures.size(); i++) {
        const auto&[texType, texture] = textures[i];
        TextureKey key(bindingKey, texType);
        auto it = m_BoundTextures.find(key);
        if (it == m_BoundTextures.end()) {
            it = m_BoundTextures.emplace(key, std::vector<BoundTexture>()).first;
        }
        it->second.push_back(metadata[i]);
    }
}

auto Material::CreateInstance(BindingKey) -> MaterialInstance {
    auto newInstanceID = m_InstanceCount++;
    m_MaterialUBOs.resize(m_InstanceCount);

    /// TODO: this is not generic yet, will fix later and add proper support for
    /// material instances with different textures and uniform buffers
    MaterialUBO ubo{
            glm::vec4(0.0f),
            glm::vec4(0.0f),
            glm::vec4(0.0f),
            32,
            -1, -1, -1
    };
    for (const auto&[texKey, textures] : m_BoundTextures) {
        switch (texKey.TextureType()) {
            case Texture2D::Type::DIFFUSE:
                if (!textures.empty()) ubo.diffuseTexIdx = textures[0].samplerIdx;
                break;
            case Texture2D::Type::SPECULAR:
                if (!textures.empty()) ubo.specularTexIdx = textures[0].samplerIdx;
                break;
        }
    }

    m_MaterialUBOs[newInstanceID] = ubo;
    SetDynamicUniforms<MaterialUBO>(m_MaterialBinding, newInstanceID, {ubo});
    return MaterialInstance(this, newInstanceID);
}

