#include "Material.h"
#include "Renderer.h"


Material::Material(const std::unique_ptr<ShaderProgram> &vs, const std::unique_ptr<ShaderProgram> &fs) {
    VertexLayout vertexLayout{
            VertexAttribute(ShaderType::Float3),
            VertexAttribute(ShaderType::Float3),
            VertexAttribute(ShaderType::Float2)
    };

    DescriptorLayout descriptorLayout{
            DescriptorBinding(DescriptorType::UniformBuffer),
            DescriptorBinding(DescriptorType::Texture)
    };

    m_Pipeline = Pipeline::Create(Renderer::GetRenderPass(), *vs, *fs, vertexLayout, descriptorLayout, {}, true);
}