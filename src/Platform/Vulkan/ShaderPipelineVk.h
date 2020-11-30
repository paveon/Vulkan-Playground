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
    std::set<BindingKey, std::less<>> m_UniformBindings;
    std::unordered_map<BindingKey, std::vector<VkImageView>> m_BoundTextures;
    std::unordered_map<BindingKey, UniformBufferVk> m_DefaultUBs;

    VkCommandBuffer m_CmdBuffer = nullptr;
    uint32_t m_ImageIndex = 0;
    uint32_t m_SubpassIndex = 0;

    /// ShaderPipeline combines descriptor sets from all shader modules
    std::unordered_map<uint32_t, VkPushConstantRange> m_PushRanges;
    std::map<ShaderType, vk::ShaderModule *> m_ShaderModules;

    auto createInfo() const -> std::vector<VkPipelineShaderStageCreateInfo>;

public:
    ShaderPipelineVk(std::string name,
                     const std::vector<std::pair<const char *, ShaderType>> &shaders,
                     const std::unordered_set<BindingKey> &perObjectUniforms, const RenderPass &renderPass,
                     uint32_t subpassIndex, std::pair<VkCullModeFlags, VkFrontFace> culling,
                     DepthState depthState);

    auto VertexBindings() const -> const auto & {
        static std::vector<vk::ShaderModule::VertexBinding> emptyBindings;
        auto it = m_ShaderModules.find(ShaderType::VERTEX_SHADER);
        if (it != m_ShaderModules.end()) {
            return it->second->GetInputBindings();
        } else {
            return emptyBindings;
        }
    }

//    auto DescriptorBindings() const -> const auto & { return m_DescriptorBindings; }

    void Recreate(const RenderPass &renderPass) override;

    void AllocateResources() override;

    void Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex, std::optional<uint32_t> materialID);

    auto BindTextures(const std::vector<const Texture2D *> &textures,
                      BindingKey bindingKey) -> std::vector<uint32_t> override;

    auto BindTextures(const std::vector<VkImageView> &textures,
                      BindingKey bindingKey) -> std::vector<uint32_t>;

    auto BindCubemap(const TextureCubemap* texture, BindingKey bindingKey) -> uint32_t override;

    void BindUniformBuffer(const UniformBuffer *buffer, BindingKey bindingKey) override;

    template<typename T>
    void PushConstants(VkCommandBuffer cmdBuffer, uint32_t index, const T& data) const {
        auto it = m_PushRanges.find(index);
        if (it == m_PushRanges.end()) {
            /// TODO: warning instead?
            return;
//            std::ostringstream msg;
//            msg << "[ShaderPipelineVk::PushConstants] Push constant with index '" << index << "' doesn't exist";
//            throw std::runtime_error(msg.str().c_str());
        }

        const auto& range = it->second;
        if (range.size != sizeof(T)) {
            std::ostringstream msg;
            msg << "[ShaderPipelineVk::PushConstants] Push constant(" << index << ") expects " << range.size
            << " bytes. Got " << sizeof(T) << " bytes instead";
            throw std::runtime_error(msg.str().c_str());
        }
        vkCmdPushConstants(cmdBuffer, m_PipelineLayout->data(), range.stageFlags, range.offset, range.size, &data);
    }

    void SetDynamicOffsets(uint32_t set, const std::vector<uint32_t> &dynamicOffsets) const override;

    void SetDynamicOffsets(uint32_t objectIndex,
                           const std::unordered_map<BindingKey, uint32_t> &customOffsets) const;

    void SetUniformData(uint32_t materialID,
                        BindingKey bindingKey,
                        const void *objectData, size_t objectCount) override;
};


#endif //VULKAN_SHADERPROGRAMVK_H
