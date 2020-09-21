#include "PipelineVk.h"

#include <Engine/Application.h>

#include <utility>
#include "RenderPassVk.h"
#include "ShaderProgramVk.h"
#include "UniformBufferVk.h"
#include "TextureVk.h"


static auto ShaderTypeToVkFormat(ShaderType type) -> VkFormat {
    switch (type) {
        case ShaderType::Float:
            return VK_FORMAT_R32_SFLOAT;
        case ShaderType::Float2:
            return VK_FORMAT_R32G32_SFLOAT;
        case ShaderType::Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case ShaderType::Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ShaderType::UInt:
            return VK_FORMAT_R8G8B8A8_UNORM;
    }

    throw std::runtime_error("Unknown shader data type");
}


static auto DescriptorTypeToVk(DescriptorType type) -> VkDescriptorType {
    switch (type) {
        case DescriptorType::Texture:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }

    throw std::runtime_error("Unknown descriptor type");
}


void PipelineVk::Recreate(const RenderPass &renderPass) {
    auto imgCount = m_Context.Swapchain().ImageCount();

    for (auto &m_PoolSize : m_PoolSizes)
        m_PoolSize.descriptorCount = imgCount;

    std::vector<VkDescriptorSetLayout> layouts(imgCount, m_DescriptorSetLayout->data());
    m_DescriptorPool = m_Device.createDescriptorPool(m_PoolSizes, imgCount);
    m_DescriptorSets = m_Device.createDescriptorSets(*m_DescriptorPool, layouts);

    m_PipelineCreateInfo.renderPass = (VkRenderPass) renderPass.VkHandle();
    m_Pipeline = m_Device.createPipeline(m_PipelineCreateInfo, m_Cache->data());
}


PipelineVk::PipelineVk(
        const RenderPassVk &renderPass,
        const ShaderProgramVk &vertexShader,
        const ShaderProgramVk &fragShader,
        const VertexLayout &vertexLayout,
        DescriptorLayout descriptorLayout,
        const std::vector<PushConstant> &pushConstants,
        bool enableDepthTest) :
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()),
        m_DescriptorLayout(std::move(descriptorLayout)) {

    auto imgCount = m_Context.Swapchain().ImageCount();

    m_Sampler = m_Device.createSampler(0.0);

    for (size_t bindingIdx = 0; bindingIdx < m_DescriptorLayout.size(); bindingIdx++) {
        VkDescriptorType type = DescriptorTypeToVk(m_DescriptorLayout[bindingIdx].type);

        m_PoolSizes.emplace_back();
        m_PoolSizes.back().type = type;
        m_PoolSizes.back().descriptorCount = imgCount;

        m_Bindings.emplace_back();
        VkDescriptorSetLayoutBinding &binding = m_Bindings.back();
        binding.descriptorType = type;
        binding.binding = bindingIdx;
        binding.descriptorCount = 1;
        switch (type) {
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;

            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                break;

            default:
                throw std::runtime_error("Unsupported descriptor type");
        }
    }

    m_DescriptorPool = m_Device.createDescriptorPool(m_PoolSizes, imgCount);
    m_DescriptorSetLayout = m_Device.createDescriptorSetLayout(m_Bindings);

    std::vector<VkDescriptorSetLayout> layouts(imgCount, m_DescriptorSetLayout->data());
    m_DescriptorSets = m_Device.createDescriptorSets(*m_DescriptorPool, layouts);
    m_Cache = m_Device.createPipelineCache(0, nullptr);

    uint32_t currentOffset = 0;
    m_Ranges.resize(pushConstants.size());
    for (size_t i = 0; i < m_Ranges.size(); i++) {
        m_Ranges[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        m_Ranges[i].size = pushConstants[i].size;
        m_Ranges[i].offset = currentOffset;
        currentOffset += pushConstants[i].size;
    }

    m_PipelineLayout = m_Device.createPipelineLayout({m_DescriptorSetLayout->data()}, {m_Ranges});

    m_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_InputAssembly.primitiveRestartEnable = VK_FALSE;

    m_Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_Rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Fill polygons
    m_Rasterizer.lineWidth = 1.0f; //Single fragment line width
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

    uint32_t vertexSize = 0;
    uint32_t location = 0;
    for (const VertexAttribute &attribute : vertexLayout) {
        VkVertexInputAttributeDescription inputAttribute = {};
        inputAttribute.location = location++;
        inputAttribute.binding = 0; //TODO;
        inputAttribute.format = ShaderTypeToVkFormat(attribute.format);
        inputAttribute.offset = vertexSize;
        m_VertexInputAttributes.push_back(inputAttribute);
        vertexSize += ShaderTypeSize(attribute.format);
    }

    m_VertexInputBindDescription.binding = 0;
    m_VertexInputBindDescription.stride = vertexSize;
    m_VertexInputBindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    m_VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_VertexInputState.vertexBindingDescriptionCount = 1;
    m_VertexInputState.pVertexBindingDescriptions = &m_VertexInputBindDescription;
    m_VertexInputState.vertexAttributeDescriptionCount = m_VertexInputAttributes.size();
    m_VertexInputState.pVertexAttributeDescriptions = m_VertexInputAttributes.data();

    m_ShaderStages[0] = vertexShader.createInfo(VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderStages[1] = fragShader.createInfo(VK_SHADER_STAGE_FRAGMENT_BIT);

    m_PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    m_PipelineCreateInfo.layout = m_PipelineLayout->data();
    m_PipelineCreateInfo.renderPass = renderPass.data();
    m_PipelineCreateInfo.pInputAssemblyState = &m_InputAssembly;
    m_PipelineCreateInfo.pRasterizationState = &m_Rasterizer;
    m_PipelineCreateInfo.pColorBlendState = &m_ColorBlendState;
    m_PipelineCreateInfo.pMultisampleState = &m_Multisampling;
    m_PipelineCreateInfo.pViewportState = &m_ViewportState;
    m_PipelineCreateInfo.pDepthStencilState = &m_DepthStencil;
    m_PipelineCreateInfo.pDynamicState = &m_DdynamicState;
    m_PipelineCreateInfo.stageCount = m_ShaderStages.size();
    m_PipelineCreateInfo.pStages = m_ShaderStages.data();
    m_PipelineCreateInfo.pVertexInputState = &m_VertexInputState;

    m_Pipeline = m_Device.createPipeline(m_PipelineCreateInfo, m_Cache->data());
}


void PipelineVk::Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex) const {
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayout->data(), 0, 1, &m_DescriptorSets->get(imageIndex), 0, nullptr);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->data());
}

void PipelineVk::BindUniformBuffer(const UniformBuffer &buffer, uint32_t binding) const {
    std::vector<VkDescriptorBufferInfo> bufferInfo(m_DescriptorSets->size());
    std::vector<VkWriteDescriptorSet> descriptorWrites(m_DescriptorSets->size());
    auto minOffset = m_Device.properties().limits.minUniformBufferOffsetAlignment;
    auto roundedSize = roundUp(buffer.ObjectSize(), minOffset);
    for (size_t i = 0; i < m_DescriptorSets->size(); i++) {
        bufferInfo[i].buffer = (VkBuffer)buffer.VkHandle();
        bufferInfo[i].offset = roundedSize * i;
        bufferInfo[i].range = buffer.ObjectSize();

        descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[i].dstSet = m_DescriptorSets->get(i);
        descriptorWrites[i].dstBinding = binding;
        descriptorWrites[i].dstArrayElement = 0;
        descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[i].descriptorCount = 1;
        descriptorWrites[i].pBufferInfo = &bufferInfo[i];
    }

    vkUpdateDescriptorSets(m_Device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void PipelineVk::BindTexture(const Texture2D &texture, uint32_t binding) const {
    const auto &textureVk = static_cast<const Texture2DVk &>(texture);

    VkDescriptorImageInfo fontDescriptor = {};
    fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    fontDescriptor.imageView = textureVk.View().data();
    fontDescriptor.sampler = m_Sampler->data();

    std::vector<VkWriteDescriptorSet> writeDescriptorSets(m_DescriptorSets->size());
    for (uint32_t i = 0; i < m_DescriptorSets->size(); i++) {
        writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[i].dstSet = m_DescriptorSets->get(i);
        writeDescriptorSets[i].dstBinding = binding;
        writeDescriptorSets[i].dstArrayElement = 0;
        writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[i].descriptorCount = 1;
        writeDescriptorSets[i].pImageInfo = &fontDescriptor;
    }

    vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}

void PipelineVk::BindTexture(const vk::ImageView &texture, uint32_t binding) const {
    VkDescriptorImageInfo fontDescriptor = {};
    fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    fontDescriptor.imageView = texture.data();
    fontDescriptor.sampler = m_Sampler->data();

    std::vector<VkWriteDescriptorSet> writeDescriptorSets(m_DescriptorSets->size());
    for (uint32_t i = 0; i < m_DescriptorSets->size(); i++) {
        writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[i].dstSet = m_DescriptorSets->get(i);
        writeDescriptorSets[i].dstBinding = binding;
        writeDescriptorSets[i].dstArrayElement = 0;
        writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[i].descriptorCount = 1;
        writeDescriptorSets[i].pImageInfo = &fontDescriptor;
    }

    vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}
