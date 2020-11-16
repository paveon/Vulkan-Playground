#include "Material.h"

#include <utility>
#include "Renderer.h"


Material::Material(std::string name, const std::shared_ptr<ShaderProgram> &program) : m_Name(std::move(name)) {
    m_VertexLayout = program->VertexLayout();
    for (const auto &ubo : program->UniformObjects()) {
        Uniform uniform{{}, ubo.size, ubo.dynamic};
        if (!ubo.dynamic) {
            uniform.data.resize(ubo.size);
        }
        m_UniformData.emplace(ubo.key, uniform);
    }

    m_Pipeline = Pipeline::Create(Renderer::GetRenderPass(), *program, {}, true);
}

void Material::AllocateResources(uint32_t objectCount) {
    m_Pipeline->AllocateResources(objectCount);
}

void Material::UpdateUniforms() {
    for (auto& [key, uniform] : m_UniformData) {
        size_t requiredDataSize = uniform.dynamic ? uniform.objectSize * m_ObjectCount : uniform.objectSize;
        uint32_t objectCount = uniform.dynamic ? m_ObjectCount : 1;
        if (uniform.data.size() < requiredDataSize) {
            uniform.data.resize(requiredDataSize);
        }
        m_Pipeline->SetUniformData(key, uniform.data.data(), objectCount);
    }
}
