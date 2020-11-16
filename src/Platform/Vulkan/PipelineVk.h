#ifndef VULKAN_PIPELINEVK_H
#define VULKAN_PIPELINEVK_H

#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/vulkan_wrappers.h>
#include "UniformBufferVk.h"

class ShaderProgramVk;
class RenderPassVk;
class GfxContextVk;
class Device;

class PipelineVk : public Pipeline {
    struct UniformResource {
        uint32_t objectSize;

    };

private:
    GfxContextVk& m_Context;
    Device& m_Device;

    vk::PipelineCache* m_Cache = nullptr;
    vk::DescriptorPool* m_DescriptorPool = nullptr;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
    vk::DescriptorSets* m_DescriptorSets = nullptr;
    vk::Sampler* m_Sampler = nullptr;

    vk::PipelineLayout* m_PipelineLayout = nullptr;
    vk::Pipeline* m_Pipeline = nullptr;

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

    std::unordered_map<uint32_t, UniformBufferVk> m_UniformResources;

    VkCommandBuffer m_CmdBuffer = nullptr;
    uint32_t m_ImageIndex = 0;


public:
    PipelineVk(const RenderPass& renderPass,
               const ShaderProgram& program,
               const std::vector<PushConstant>& pushConstants,
               bool enableDepthTest);

    void Recreate(const RenderPass &renderPass) override;

    void AllocateResources(uint32_t objectCount) override;

    void Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex) override;

//    void BindTexture(const vk::ImageView &texture, uint32_t set, uint32_t binding, uint32_t textureCount) const;

    void BindTextures(const std::vector<const Texture2D *> &textures, uint32_t set, uint32_t binding) const override;

    void BindUniformBuffer(const UniformBuffer& buffer, uint32_t set, uint32_t binding) const override;

    void PushConstants(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void* data) const override {
       vkCmdPushConstants(cmdBuffer, m_PipelineLayout->data(), VK_SHADER_STAGE_VERTEX_BIT, offset, size, data);
    }

    void SetDynamicOffsets(uint32_t set, const std::vector<uint32_t> &dynamicOffsets) const override;

    void SetDynamicOffsets(uint32_t objectIndex) const override;

    void SetUniformData(uint32_t key, const void *objectData, size_t objectCount) override;
};


#endif //VULKAN_PIPELINEVK_H
