#ifndef VULKAN_PIPELINEVK_H
#define VULKAN_PIPELINEVK_H

#include <Engine/Renderer/Pipeline.h>
#include <Engine/Renderer/vulkan_wrappers.h>
#include <Engine/Application.h>
#include "GraphicsContextVk.h"

class ShaderProgramVk;

class RenderPassVk;

class PipelineVk : public Pipeline {
private:
    GfxContextVk& m_Context;
    Device& m_Device;

    vk::PipelineCache* m_Cache = nullptr;
    vk::DescriptorPool* m_DescriptorPool = nullptr;
    vk::DescriptorSetLayout* m_DescriptorSetLayout = nullptr;
    vk::DescriptorSets* m_DescriptorSets = nullptr;
    vk::Sampler* m_Sampler = nullptr;

    vk::PipelineLayout* m_PipelineLayout = nullptr;
    vk::Pipeline* m_Pipeline = nullptr;

    DescriptorLayout m_DescriptorLayout;
    std::vector<VkDescriptorPoolSize> m_PoolSizes;
    std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
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
    VkVertexInputBindingDescription m_VertexInputBindDescription = {};
    VkPipelineVertexInputStateCreateInfo m_VertexInputState = {};
    VkGraphicsPipelineCreateInfo m_PipelineCreateInfo = {};

    std::array<VkDynamicState, 2> m_DynamicStateEnables = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    std::array<VkPipelineShaderStageCreateInfo, 2> m_ShaderStages = {};


public:
    PipelineVk(const RenderPassVk& renderPass, const ShaderProgramVk& vertexShader,
               const ShaderProgramVk& fragShader, const VertexLayout& vertexLayout,
               DescriptorLayout  descriptorLayout, const std::vector<PushConstant>& pushConstants,
               bool enableDepthTest);

    void Recreate(const RenderPass &renderPass) override;

    void Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex) const override;

    void BindTexture(const vk::ImageView& texture, uint32_t binding) const;

    void BindTexture(const Texture2D& texture, uint32_t binding) const override;

    void BindUniformBuffer(const UniformBuffer& buffer, uint32_t binding) const override;

    void PushConstants(VkCommandBuffer cmdBuffer, uint32_t offset, uint32_t size, const void* data) const override {
       vkCmdPushConstants(cmdBuffer, m_PipelineLayout->data(), VK_SHADER_STAGE_VERTEX_BIT, offset, size, data);
    }
};


#endif //VULKAN_PIPELINEVK_H
