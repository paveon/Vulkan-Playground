#ifndef VULKAN_SHADERPROGRAMVK_H
#define VULKAN_SHADERPROGRAMVK_H


#include <Engine/Renderer/ShaderPipeline.h>
#include <Engine/Renderer/vulkan_wrappers.h>
#include <unordered_map>
#include "UniformBufferVk.h"
#include "RenderPassVk.h"

class GfxContextVk;


struct PushConstantKey {
    uint64_t value = 0;

    PushConstantKey() = default;

    explicit PushConstantKey(uint64_t value) : value(value) {}

    PushConstantKey(VkShaderStageFlags stages, uint32_t index) : value(((uint64_t)stages << 32ul) + index) {}

    auto Stage() const { return (value & 0xFFFFFFFF00000000u) >> 32u; }

    auto Index() const { return value & 0x00000000FFFFFFFFu; }

    auto operator()() const -> std::size_t { return value; }

    auto operator==(const PushConstantKey &other) const -> bool { return value == other.value; }

    auto operator<(const PushConstantKey &other) const -> bool { return value < other.value; }
};


namespace std {
    template<>
    struct hash<PushConstantKey> {
        auto operator()(const PushConstantKey &k) const -> std::size_t { return std::hash<uint64_t>()(k.value); }
    };
}



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
    std::vector<VkPipelineColorBlendAttachmentState> m_BlendStates = {};
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
    std::unordered_map<PushConstantKey, VkPushConstantRange> m_PushRanges;
    std::map<ShaderType, vk::ShaderModule *> m_ShaderModules;

    auto createInfo() const -> std::vector<VkPipelineShaderStageCreateInfo>;

    auto BindImageViews(const std::vector<VkImageView> &textures,
                        BindingKey bindingKey,
                        SamplerBinding::Type type) -> std::vector<uint32_t>;

public:
    ShaderPipelineVk(std::string name,
                     const std::map<ShaderType, const char *> &shaders,
                     const std::unordered_set<BindingKey> &perObjectUniforms,
                     const RenderPassVk &renderPass,
                     uint32_t subpassIndex,
                     VkPrimitiveTopology topology,
                     std::pair<VkCullModeFlags, VkFrontFace> culling,
                     DepthState depthState,
                     MultisampleState msState);

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

    void Bind(VkCommandBuffer cmdBuffer);

    void BindDescriptorSets(uint32_t imageIndex, std::optional<uint32_t> materialID);

    auto BindTextures2D(const std::vector<const Texture2D *> &textures,
                        BindingKey bindingKey) -> std::vector<uint32_t> override;

    auto BindCubemaps(const std::vector<const TextureCubemap *> &cubemaps,
                      BindingKey bindingKey) -> std::vector<uint32_t> override;

    auto BindTextures2D(const std::vector<VkImageView> &textures, BindingKey bindingKey) -> std::vector<uint32_t> {
        return BindImageViews(textures, bindingKey, SamplerBinding::Type::SAMPLER_2D);
    }

    auto BindCubemaps(const std::vector<VkImageView> &cubemaps, BindingKey bindingKey) -> std::vector<uint32_t> {
        return BindImageViews(cubemaps, bindingKey, SamplerBinding::Type::SAMPLER_CUBE);
    }

    void BindUniformBuffer(const UniformBuffer *buffer, BindingKey bindingKey) override;

    template<typename T>
    void PushConstants(VkCommandBuffer cmdBuffer, PushConstantKey key, const T &data) const {
        auto it = m_PushRanges.find(key);
        if (it == m_PushRanges.end()) {
            /// TODO: warning instead?
            return;
//            std::ostringstream msg;
//            msg << "[ShaderPipelineVk::PushConstants] Push constant with index '" << index << "' doesn't exist";
//            throw std::runtime_error(msg.str().c_str());
        }

        const auto &range = it->second;
        if (range.size != sizeof(T)) {
            std::ostringstream msg;
            msg << "[ShaderPipelineVk::PushConstants] Push constant(" << key.Index() << ") expects " << range.size
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
