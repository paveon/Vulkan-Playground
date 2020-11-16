#include "PipelineVk.h"

#include <Engine/Renderer/Renderer.h>

#include "RenderPassVk.h"
#include "ShaderProgramVk.h"
#include "UniformBufferVk.h"
#include "TextureVk.h"


static auto ShaderTypeToVkFormat(ShaderAttributeType type) -> VkFormat {
    switch (type) {
        case ShaderAttributeType::Float:
            return VK_FORMAT_R32_SFLOAT;
        case ShaderAttributeType::Float2:
            return VK_FORMAT_R32G32_SFLOAT;
        case ShaderAttributeType::Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderAttributeType::Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ShaderAttributeType::UInt:
            return VK_FORMAT_R8G8B8A8_UNORM;
    }

    throw std::runtime_error("[ShaderTypeToVkFormat] Unsupported shader vertex attribute type");
}

static auto VertexInputRateToVk(VertexBindingInputRate rate) -> VkVertexInputRate {
    switch (rate) {
        case VertexBindingInputRate::PER_VERTEX:
            return VK_VERTEX_INPUT_RATE_VERTEX;
        case VertexBindingInputRate::PER_INSTANCE:
            return VK_VERTEX_INPUT_RATE_INSTANCE;
    }

    throw std::runtime_error("[VertexInputRateToVk] Unsupported input rate");
}

static auto DescriptorTypeToVk(DescriptorType type) -> VkDescriptorType {
    switch (type) {
        case DescriptorType::Texture:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::UniformBufferDynamic:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    }

    throw std::runtime_error("[DescriptorTypeToVk] Unsupported descriptor type");
}


void PipelineVk::Recreate(const RenderPass &renderPass) {
    auto imgCount = m_Context.Swapchain().ImageCount();

    for (auto &m_PoolSize : m_PoolSizes)
        m_PoolSize.descriptorCount = imgCount;

//    std::vector<VkDescriptorSetLayout> layouts(imgCount, m_DescriptorSetLayouts->data());
    std::vector<VkDescriptorSetLayout> setCreationLayouts;
    for (size_t imageIdx = 0; imageIdx < imgCount; imageIdx++) {
        for (const auto &setLayout : m_DescriptorSetLayouts) {
            setCreationLayouts.emplace_back(setLayout);
        }
    }

    m_DescriptorPool = m_Device.createDescriptorPool(m_PoolSizes, imgCount);
    m_DescriptorSets = m_Device.createDescriptorSets(*m_DescriptorPool, setCreationLayouts);

    m_PipelineCreateInfo.renderPass = (VkRenderPass) renderPass.VkHandle();
    m_Pipeline = m_Device.createPipeline(m_PipelineCreateInfo, m_Cache->data());
}


PipelineVk::PipelineVk(
        const RenderPass &renderPass,
        const ShaderProgram &program,
        const std::vector<PushConstant> &pushConstants,
        bool enableDepthTest) :
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()) {

    const auto &renderPassVk = static_cast<const RenderPassVk &>(renderPass);
    const auto &programVk = static_cast<const ShaderProgramVk &>(program);

    auto imgCount = m_Context.Swapchain().ImageCount();

    m_Sampler = m_Device.createSampler(0.0);
    const auto &descriptorSetLayouts = programVk.DescriptorSetLayouts();

    for (const auto &uniform : programVk.UniformObjects()) {
        m_UniformResources.emplace(uniform.key, UniformBufferVk(uniform.size, uniform.dynamic));
    }

    std::vector<std::vector<VkDescriptorSetLayoutBinding>> layoutDescriptions;
    for (const auto &[setIdx, setLayout] : descriptorSetLayouts) {
        layoutDescriptions.emplace_back();
        std::vector<VkDescriptorSetLayoutBinding> &bindings(layoutDescriptions.back());
        for (const auto&[bindingIdx, setBinding] : setLayout) {
            /// TODO: how to deal with arrays of textures without specified size?
            uint32_t descriptorCount = setBinding.count == 0 ? 10 : setBinding.count;
            m_PoolSizes.emplace_back();
            m_PoolSizes.back().type = setBinding.type;
            m_PoolSizes.back().descriptorCount = descriptorCount;

            bindings.emplace_back();
            VkDescriptorSetLayoutBinding &binding = bindings.back();
            binding.descriptorType = setBinding.type;
            binding.binding = bindingIdx;
            binding.descriptorCount = descriptorCount;

            /// TODO: this is hardcoded, must store the info somewhere in the shader program
            switch (setBinding.type) {
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;

                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;

                default:
                    throw std::runtime_error("Unsupported descriptor type");
            }
        }
    }

    m_DescriptorPool = m_Device.createDescriptorPool(m_PoolSizes, imgCount * layoutDescriptions.size());
    for (const auto &bindings : layoutDescriptions) {
        m_DescriptorSetLayouts.emplace_back(m_Device.createDescriptorSetLayout(bindings)->data());
    }

    /// Create multiple sets for each layout to accomodate for double/triple buffering
    /// Layouts are intertwined so we can bind all sets for one image at the same time
    std::vector<VkDescriptorSetLayout> setCreationLayouts;
    for (size_t imageIdx = 0; imageIdx < imgCount; imageIdx++) {
        for (const auto &setLayout : m_DescriptorSetLayouts) {
            setCreationLayouts.emplace_back(setLayout);
        }
    }

    m_DescriptorSets = m_Device.createDescriptorSets(*m_DescriptorPool, setCreationLayouts);

    m_Cache = m_Device.createPipelineCache(0, nullptr);

    uint32_t currentOffset = 0;
    m_Ranges.resize(pushConstants.size());
    for (size_t i = 0; i < m_Ranges.size(); i++) {
        m_Ranges[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        m_Ranges[i].size = pushConstants[i].size;
        m_Ranges[i].offset = currentOffset;
        currentOffset += pushConstants[i].size;
    }

    m_PipelineLayout = m_Device.createPipelineLayout(m_DescriptorSetLayouts, m_Ranges);

    m_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_InputAssembly.primitiveRestartEnable = VK_FALSE;

    m_Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_Rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Fill polygons
    m_Rasterizer.lineWidth = 1.0f; //Single fragment line width
//    m_Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //Back-face culling
    m_Rasterizer.cullMode = VK_CULL_MODE_NONE; //Back-face culling
    m_Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //Front-facing polygons have vertices in clockwise order

    m_BlendAttachmentState.blendEnable = VK_TRUE;
    m_BlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
    m_BlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    m_BlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    m_BlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    m_BlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    m_BlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    m_BlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    m_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_ColorBlendState.attachmentCount = 1;
    m_ColorBlendState.pAttachments = &m_BlendAttachmentState;

    if (enableDepthTest) {
        m_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        m_DepthStencil.depthTestEnable = VK_TRUE;
        m_DepthStencil.depthWriteEnable = VK_TRUE;
        m_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
        m_DepthStencil.minDepthBounds = 0.0f; // Optional
        m_DepthStencil.maxDepthBounds = 1.0f; // Optional
        m_DepthStencil.stencilTestEnable = VK_FALSE;
        m_DepthStencil.front = {}; // Optional
        m_DepthStencil.back = {}; // Optional
    } else {
        m_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        m_DepthStencil.depthTestEnable = VK_FALSE;
        m_DepthStencil.depthWriteEnable = VK_FALSE;
        m_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        m_DepthStencil.front = m_DepthStencil.back;
        m_DepthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
    }

    m_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_ViewportState.viewportCount = 1;
    m_ViewportState.scissorCount = 1;

    m_Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; //TODO

    m_DdynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_DdynamicState.dynamicStateCount = m_DynamicStateEnables.size();
    m_DdynamicState.pDynamicStates = m_DynamicStateEnables.data();

    for (const vk::ShaderModule::VertexBinding &bindingDescription : programVk.VertexBindings()) {
        uint32_t vertexSize = 0;
        uint32_t location = 0;
        for (const auto &attribute : bindingDescription.vertexLayout) {
            VkVertexInputAttributeDescription inputAttribute = {};
            inputAttribute.location = location++;
            inputAttribute.binding = bindingDescription.binding;
            inputAttribute.format = attribute.format;
            inputAttribute.offset = vertexSize;
            m_VertexInputAttributes.push_back(inputAttribute);
            vertexSize += attribute.size;
        }
        m_VertexInputBindings.emplace_back(VkVertexInputBindingDescription{
                bindingDescription.binding,
                vertexSize,
                bindingDescription.inputRate
        });
    }


    m_VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_VertexInputState.vertexBindingDescriptionCount = m_VertexInputBindings.size();
    m_VertexInputState.pVertexBindingDescriptions = m_VertexInputBindings.data();
    m_VertexInputState.vertexAttributeDescriptionCount = m_VertexInputAttributes.size();
    m_VertexInputState.pVertexAttributeDescriptions = m_VertexInputAttributes.data();

    m_ShaderStagesCreateInfo = programVk.createInfo();

    m_PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    m_PipelineCreateInfo.layout = m_PipelineLayout->data();
    m_PipelineCreateInfo.renderPass = renderPassVk.data();
    m_PipelineCreateInfo.pInputAssemblyState = &m_InputAssembly;
    m_PipelineCreateInfo.pRasterizationState = &m_Rasterizer;
    m_PipelineCreateInfo.pColorBlendState = &m_ColorBlendState;
    m_PipelineCreateInfo.pMultisampleState = &m_Multisampling;
    m_PipelineCreateInfo.pViewportState = &m_ViewportState;
    m_PipelineCreateInfo.pDepthStencilState = &m_DepthStencil;
    m_PipelineCreateInfo.pDynamicState = &m_DdynamicState;
    m_PipelineCreateInfo.stageCount = m_ShaderStagesCreateInfo.size();
    m_PipelineCreateInfo.pStages = m_ShaderStagesCreateInfo.data();
    m_PipelineCreateInfo.pVertexInputState = &m_VertexInputState;

    m_Pipeline = m_Device.createPipeline(m_PipelineCreateInfo, m_Cache->data());
}


void PipelineVk::AllocateResources(uint32_t objectCount) {
    auto imgCount = m_Context.Swapchain().ImageCount();
    auto layoutCount = m_DescriptorSetLayouts.size();

    std::vector<VkDescriptorBufferInfo> bufferInfo(imgCount * m_UniformResources.size());
    std::vector<VkWriteDescriptorSet> descriptorWrites(imgCount * m_UniformResources.size());
    size_t uniformIdx = 0;
    for (auto&[key, uniform] : m_UniformResources) {
        uint32_t setIdx = (key >> 16u);
        uint32_t bindingIdx = (key & 0x0000FFFFu);

        VkDescriptorType type = uniform.IsDynamic() ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                                    : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        uniform.Allocate(uniform.IsDynamic() ? objectCount : 1);
        for (size_t i = 0; i < imgCount; i++) {
            size_t idx = (imgCount * uniformIdx) + i;
            bufferInfo[idx].buffer = uniform.VkHandle();
            bufferInfo[idx].offset = uniform.BaseOffset() + (uniform.SubSize() * i);
            bufferInfo[idx].range = uniform.IsDynamic() ? uniform.ObjectSizeAligned() : uniform.SubSize();

            descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[idx].dstSet = m_DescriptorSets->get((i * layoutCount) + setIdx);
            descriptorWrites[idx].dstBinding = bindingIdx;
            descriptorWrites[idx].dstArrayElement = 0;
            descriptorWrites[idx].descriptorType = type;
            descriptorWrites[idx].descriptorCount = 1;
            descriptorWrites[idx].pBufferInfo = &bufferInfo[idx];
        }
        uniformIdx++;
    }

    vkUpdateDescriptorSets(m_Device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}


void PipelineVk::Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex) {
    m_CmdBuffer = cmdBuffer;
    m_ImageIndex = imageIndex;

    uint32_t setCount = m_DescriptorSetLayouts.size();  /// Number of layouts == stride
    uint32_t firstSet = setCount * m_ImageIndex;

    vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->data());

    std::vector<uint32_t> dynamicOffsets;
    for (const auto&[key, uniform] : m_UniformResources) {
        if (uniform.IsDynamic())
            dynamicOffsets.emplace_back(0);
    }

    /// Bind all necessary sets for this image at once
    vkCmdBindDescriptorSets(m_CmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayout->data(),
                            0,
                            setCount,
                            &m_DescriptorSets->get(firstSet),
                            dynamicOffsets.size(),
                            dynamicOffsets.data());
}


void PipelineVk::SetDynamicOffsets(uint32_t set, const std::vector<uint32_t> &dynamicOffsets) const {
    uint32_t setIndex = m_DescriptorSetLayouts.size() * m_ImageIndex + set;
    vkCmdBindDescriptorSets(m_CmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayout->data(),
                            set,
                            1,
                            &m_DescriptorSets->get(setIndex),
                            dynamicOffsets.size(),
                            dynamicOffsets.data());
}


void PipelineVk::SetDynamicOffsets(uint32_t objectIndex) const {
    /// TODO: figure out, how exactly batching of 'vkCmdBindDescriptorSets'
    /// TODO: works in case we have more dynamic uniform buffers per material
    uint32_t setCount = m_DescriptorSetLayouts.size();  /// Number of layouts == stride
    uint32_t setIndex = setCount * m_ImageIndex;
    std::vector<uint32_t> dynamicOffsets;
    for (const auto&[key, uniform] : m_UniformResources) {
        if (uniform.IsDynamic()) {
            dynamicOffsets.emplace_back(uniform.ObjectSizeAligned() * objectIndex);
        }
    }

    vkCmdBindDescriptorSets(m_CmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayout->data(),
                            0,
                            setCount,
                            &m_DescriptorSets->get(setIndex),
                            dynamicOffsets.size(),
                            dynamicOffsets.data());
}


void PipelineVk::BindUniformBuffer(const UniformBuffer &buffer, uint32_t set, uint32_t binding) const {
    auto bufferVk = static_cast<const UniformBufferVk&>(buffer);
    auto imgCount = m_Context.Swapchain().ImageCount();
    auto layoutCount = m_DescriptorSetLayouts.size();

    std::vector<VkDescriptorBufferInfo> bufferInfo(imgCount);
    std::vector<VkWriteDescriptorSet> descriptorWrites(imgCount);
    for (size_t i = 0; i < imgCount; i++) {
        bufferInfo[i].buffer = bufferVk.VkHandle();
        bufferInfo[i].offset = bufferVk.BaseOffset() + (bufferVk.SubSize() * i);
        bufferInfo[i].range = bufferVk.SubSize();

        descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[i].dstSet = m_DescriptorSets->get((i * layoutCount) + set);
        descriptorWrites[i].dstBinding = binding;
        descriptorWrites[i].dstArrayElement = 0;
        descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[i].descriptorCount = 1;
        descriptorWrites[i].pBufferInfo = &bufferInfo[i];
    }

    vkUpdateDescriptorSets(m_Device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void PipelineVk::BindTextures(const std::vector<const Texture2D *> &textures, uint32_t set, uint32_t binding) const {
    auto imgCount = m_Context.Swapchain().ImageCount();
    auto layoutCount = m_DescriptorSetLayouts.size();

    std::vector<VkDescriptorImageInfo> textureDescriptors(textures.size());
    for (size_t i = 0; i < textures.size(); i++) {
        const auto* textureVk = static_cast<const Texture2DVk*>(textures[i]);
        textureDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        textureDescriptors[i].imageView = textureVk->View().data();
        textureDescriptors[i].sampler = m_Sampler->data();
    }

    /// TODO: doesn't work if descriptor sets numbers are not continuous and starting from 0
    /// TODO: need to do internal remapping...
    std::vector<VkWriteDescriptorSet> writeDescriptorSets(imgCount);
    for (uint32_t i = 0; i < imgCount; i++) {
        writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[i].dstSet = m_DescriptorSets->get((i * layoutCount) + set);
        writeDescriptorSets[i].dstBinding = binding;
        writeDescriptorSets[i].dstArrayElement = 0;
        writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[i].descriptorCount = textures.size();
        writeDescriptorSets[i].pImageInfo = textureDescriptors.data();
    }

    vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

//void PipelineVk::BindTexture(const vk::ImageView &texture, uint32_t set, uint32_t binding, uint32_t textureCount) const {
//}

void PipelineVk::SetUniformData(uint32_t key, const void *objectData, size_t objectCount) {
    const auto &it = m_UniformResources.find(key);
    if (it != m_UniformResources.end()) {
        it->second.SetData(objectData, objectCount);
    }
}
