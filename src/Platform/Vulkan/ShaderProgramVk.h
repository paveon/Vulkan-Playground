#ifndef VULKAN_SHADERPROGRAMVK_H
#define VULKAN_SHADERPROGRAMVK_H


#include <Engine/Renderer/ShaderProgram.h>
#include <Engine/Renderer/vulkan_wrappers.h>
#include <unordered_map>


//enum class ShaderAttributeType {
//    Float,
//    Float2,
//    Float3,
//    Float4,
//    UInt
//};
//
//enum class VertexBindingInputRate {
//    PER_VERTEX = 0,
//    PER_INSTANCE = 1,
//};
//
//enum class DescriptorType {
//    UniformBuffer,
//    UniformBufferDynamic,
//    Texture
//};
//
//
//struct VertexAttribute {
//    ShaderAttributeType format;
//};
//
//struct DescriptorBinding {
//    DescriptorType type;
//    uint32_t count;
//};

//using VertexLayout = std::vector<VkFormat>;
//using DescriptorSetLayout = std::vector<DescriptorBinding>;


class ShaderProgramVk : public ShaderProgram {
    /// Shader program combines descriptor sets from all shader modules
    const std::vector<vk::ShaderModule::VertexBinding>* m_VertexBindings = nullptr;
    std::map<uint32_t, std::map<uint32_t, vk::ShaderModule::DescriptorBinding>> m_DescriptorSetLayouts;
    std::map<ShaderType, vk::ShaderModule *> m_ShaderModules;

public:
    explicit ShaderProgramVk(const std::vector<std::pair<const char *, ShaderType>>& shaders,
                             const std::vector<uint32_t>& dynamics);

    auto VertexBindings() const -> const auto& { return *m_VertexBindings; }

    auto DescriptorSetLayouts() const -> const auto& { return m_DescriptorSetLayouts; }

    auto createInfo() const -> std::vector<VkPipelineShaderStageCreateInfo>;
};


#endif //VULKAN_SHADERPROGRAMVK_H
