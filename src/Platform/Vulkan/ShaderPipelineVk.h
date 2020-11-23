#ifndef VULKAN_SHADERPROGRAMVK_H
#define VULKAN_SHADERPROGRAMVK_H


#include <Engine/Renderer/ShaderPipeline.h>
#include <Engine/Renderer/vulkan_wrappers.h>
#include <unordered_map>
#include "UniformBufferVk.h"

class GfxContextVk;

class ShaderPipelineVk : public ShaderPipeline {
    GfxContextVk &m_Context;
    Device &m_Device;

    vk::PipelineCache *m_Cache = nullptr;
    vk::DescriptorPool *m_DescriptorPool = nullptr;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
    vk::DescriptorSets *m_DescriptorSets = nullptr;
    vk::Sampler *m_Sampler = nullptr;

    vk::PipelineLayout *m_PipelineLayout = nullptr;
    vk::Pipeline *m_Pipeline = nullptr;

    std::vector<VkDescriptorPoolSize> m_PoolSizes;
    std::vector<VkPushConstantRange> m_Ranges;

    VkPipelineInputAssemblyStateCreateInfo m_InputAssembly = {};
    VkPipelineRasterizationStateCreateInfo m_Rasterizer = {};
    VkPipelineColorBlendAttachmentState m_BlendAttachmentState = {};
    VkPipelineColorBlendStateCreateInfo m_ColorBlendState = {};
    VkPipelineDepthStencilStateCreateInfo m_DepthStencil = {};
    VkPipelineViewportStateCreateInfo m_ViewportState = {};
    VkPipelineMultisampleStateCreateInfo m_Multisampling = {};
    VkPipelineDynamicStateCreateInfo m_DdynamicState = {};
    std::vector<VkVertexInputAttributeDescription> m_VertexInputAttributes;
    std::vector<VkVertexInputBindingDescription> m_VertexInputBindings;
    VkPipelineVertexInputStateCreateInfo m_VertexInputState = {};
    VkGraphicsPipelineCreateInfo m_PipelineCreateInfo = {};

    std::array<VkDynamicState, 2> m_DynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStagesCreateInfo;

    std::vector<uint32_t> m_BaseDynamicOffsets;
    std::vector<BindingKey> m_UniformSlots;
//    std::unordered_map<BindingKey, const UniformBufferVk *> m_BoundUBs;
    std::unordered_map<BindingKey, UniformBufferVk> m_DefaultUBs;
    std::unordered_map<BindingKey, std::pair<bool, const UniformBufferVk *>> m_ActiveUBs;

    std::unordered_map<MaterialKey, uint32_t> m_MaterialBufferOffsets;

    VkCommandBuffer m_CmdBuffer = nullptr;
    uint32_t m_ImageIndex = 0;
    uint32_t m_CurrentMaterialID = 0;

    /// ShaderPipeline combines descriptor sets from all shader modules
    std::map<uint32_t, std::map<uint32_t, vk::ShaderModule::DescriptorBinding>> m_DescriptorBindings;
    std::map<ShaderType, vk::ShaderModule *> m_ShaderModules;

    auto createInfo() const -> std::vector<VkPipelineShaderStageCreateInfo>;

public:
    explicit ShaderPipelineVk(std::string name,
                              const std::vector<std::pair<const char *, ShaderType>> &shaders,
                              const std::vector<BindingKey> &perObjectUniforms,
                              const RenderPass &renderPass,
                              const std::vector<PushConstant> &pushConstants,
                              bool enableDepthTest);

    auto VertexBindings() const -> const auto & {
        static std::vector<vk::ShaderModule::VertexBinding> emptyBindings;
        auto it = m_ShaderModules.find(ShaderType::VERTEX_SHADER);
        if (it != m_ShaderModules.end()) {
            return it->second->GetInputBindings();
        } else {
            return emptyBindings;
        }
    }

    auto DescriptorBindings() const -> const auto & { return m_DescriptorBindings; }

    void Recreate(const RenderPass &renderPass) override;

    void AllocateResources() override;

    void Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex, uint32_t materialID);

    auto BindTextures(const std::vector<const Texture2D *> &textures, BindingKey bindingKey) -> std::vector<uint32_t> override;

    void BindUniformBuffer(const UniformBuffer *buffer, BindingKey bindingKey) override;

//    void PushConstants(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void *data) const override {
//        vkCmdPushConstants(cmdBuffer, m_PipelineLayout->data(), VK_SHADER_STAGE_VERTEX_BIT, offset, size, data);
//    }

    void SetDynamicOffsets(uint32_t set, const std::vector<uint32_t> &dynamicOffsets) const override;

    void SetDynamicOffsets(uint32_t objectIndex,
                           const std::unordered_map<BindingKey, uint32_t> &customOffsets) const;

    void SetUniformData(uint32_t materialID, BindingKey bindingKey, const void *objectData, size_t objectCount) override;
};


#endif //VULKAN_SHADERPROGRAMVK_H
