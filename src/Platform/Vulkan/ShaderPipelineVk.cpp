#include "ShaderPipelineVk.h"

#include <Engine/Renderer/Renderer.h>
#include <Engine/Application.h>
#include "RenderPassVk.h"
#include "UniformBufferVk.h"
#include "TextureVk.h"


//static auto ShaderTypeToVkFormat(ShaderAttributeType type) -> VkFormat {
//    switch (type) {
//        case ShaderAttributeType::Float:
//            return VK_FORMAT_R32_SFLOAT;
//        case ShaderAttributeType::Float2:
//            return VK_FORMAT_R32G32_SFLOAT;
//        case ShaderAttributeType::Float3:
//            return VK_FORMAT_R32G32B32_SFLOAT;
//        case ShaderAttributeType::Float4:
//            return VK_FORMAT_R32G32B32A32_SFLOAT;
//        case ShaderAttributeType::UInt:
//            return VK_FORMAT_R8G8B8A8_UNORM;
//    }
//
//    throw std::runtime_error("[ShaderTypeToVkFormat] Unsupported shader vertex attribute type");
//}

//static auto VertexInputRateToVk(VertexBindingInputRate rate) -> VkVertexInputRate {
//    switch (rate) {
//        case VertexBindingInputRate::PER_VERTEX:
//            return VK_VERTEX_INPUT_RATE_VERTEX;
//        case VertexBindingInputRate::PER_INSTANCE:
//            return VK_VERTEX_INPUT_RATE_INSTANCE;
//    }
//
//    throw std::runtime_error("[VertexInputRateToVk] Unsupported input rate");
//}

//static auto DescriptorTypeToVk(DescriptorType type) -> VkDescriptorType {
//    switch (type) {
//        case DescriptorType::Texture:
//            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        case DescriptorType::UniformBuffer:
//            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        case DescriptorType::UniformBufferDynamic:
//            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
//    }
//
//    throw std::runtime_error("[DescriptorTypeToVk] Unsupported descriptor type");
//}


ShaderPipelineVk::ShaderPipelineVk(std::string name,
                                   const std::vector<std::pair<const char *, ShaderType>> &shaders,
                                   const std::vector<BindingKey> &perObjectUniforms,
                                   const RenderPass &renderPass,
                                   const std::vector<PushConstant> &pushConstants,
                                   bool enableDepthTest) :
        ShaderPipeline(std::move(name)),
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()) {

    auto &device = static_cast<GfxContextVk &>(Application::GetGraphicsContext()).GetDevice();
    const auto &renderPassVk = static_cast<const RenderPassVk &>(renderPass);
    auto imgCount = m_Context.Swapchain().ImageCount();

    for (const auto&[filepath, shaderType] : shaders) {
        auto *module = device.createShaderModule(filepath);

        /// Merge descriptor set layouts from all stages
        for (const auto&[setIdx, setLayout] : module->GetSetLayouts()) {
            auto &layout = m_DescriptorBindings[setIdx];
            for (const auto&[bindingIdx, binding] : setLayout) {
                if (layout.find(bindingIdx) == layout.end()) {
                    layout[bindingIdx] = binding;

                    if (binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                        binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {

                        m_UniformSlots.emplace_back(setIdx, bindingIdx);
                        const BindingKey& key = m_UniformSlots.back();

                        bool perObject = std::find(perObjectUniforms.begin(), perObjectUniforms.end(), key) !=
                                         perObjectUniforms.end();
                        m_ShaderUniforms.emplace(key, ShaderPipeline::Uniform{
                                binding.name,
                                binding.size,
                                perObject
                        });
                        std::ostringstream ubName;
                        ubName << "Default UB {" << setIdx << ";" << bindingIdx << "}";
                        m_DefaultUBs.emplace(
                                std::piecewise_construct,
                                std::forward_as_tuple(key),
                                std::forward_as_tuple(ubName.str(), binding.size, perObject)
                        );
                    }
                }
            }
        }
        m_ShaderModules[shaderType] = module;
    }
    std::sort(m_UniformSlots.begin(), m_UniformSlots.end(),
              [](const BindingKey &l, const BindingKey &r) { return l.value < r.value; });

    m_BaseDynamicOffsets.resize(m_UniformSlots.size());

    const auto &vertexBindings = m_ShaderModules[ShaderType::VERTEX_SHADER]->GetInputBindings();
    for (const auto &bindingDescription : vertexBindings) {
        uint32_t vertexSize = 0;
        uint32_t location = 0;

        for (const auto &attribute : bindingDescription.vertexLayout) {
            /// TODO: ignoring cases with multiple bindings for now
            m_VertexLayout.push_back(attribute.size);

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

    /// TODO: more robust solution, this is naive and might break
    /// Check if input of each subsequent shader stage matches output of the previous one
    auto endIt = std::prev(m_ShaderModules.end(), 1);
    for (auto it = m_ShaderModules.begin(); it != endIt;) {
        const auto &output = it->second->GetOutputBindings();
        const auto &nextStage = std::next(it, 1);
        const auto &nextInput = nextStage->second->GetInputBindings();
        const auto &outputLayout = output.front().vertexLayout;
        const auto &nextInputLayout = nextInput.front().vertexLayout;

        if (outputLayout.size() != nextInputLayout.size()) {
            std::ostringstream msg;
            msg << "[ShaderPipelineVk] Stage '" << ShaderTypeToString(it->first) << "' with '" << outputLayout.size()
                << "' output vertex attributes mismatches stage '" << ShaderTypeToString(nextStage->first)
                << "' with '" << nextInputLayout.size() << "' input vertex attributes";
            throw std::runtime_error(msg.str().c_str());
        }

        for (size_t i = 0; i < outputLayout.size(); i++) {
            if (outputLayout[i] != nextInputLayout[i]) {
                std::ostringstream msg;
                msg << "[ShaderPipelineVk] Output vertex attribute of stage '" << ShaderTypeToString(it->first)
                    << "' with location '" << i << "' mismatches input attribute of next stage '"
                    << ShaderTypeToString(nextStage->first) << "'";
                throw std::runtime_error(msg.str().c_str());
            }
        }
        it = nextStage;
    }


    m_Sampler = m_Device.createSampler(0.0);

    std::vector<std::vector<VkDescriptorSetLayoutBinding>> layoutDescriptions;
    for (const auto &[setIdx, setLayout] : m_DescriptorBindings) {
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


    m_VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_VertexInputState.vertexBindingDescriptionCount = m_VertexInputBindings.size();
    m_VertexInputState.pVertexBindingDescriptions = m_VertexInputBindings.data();
    m_VertexInputState.vertexAttributeDescriptionCount = m_VertexInputAttributes.size();
    m_VertexInputState.pVertexAttributeDescriptions = m_VertexInputAttributes.data();

    m_ShaderStagesCreateInfo = createInfo();

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


void ShaderPipelineVk::Recreate(const RenderPass &renderPass) {
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


void ShaderPipelineVk::AllocateResources() {
    uint32_t instanceCount = 0;
    std::unordered_map<uint32_t, uint32_t> instanceCountScan(m_MaterialCount);
    for (const auto&[materialID, material] : m_BoundMaterials) {
        instanceCountScan[materialID] = instanceCount;
        instanceCount += material->GetInstanceCount();
    }

    auto imgCount = m_Context.Swapchain().ImageCount();
    auto layoutCount = m_DescriptorSetLayouts.size();

    std::vector<VkDescriptorBufferInfo> bufferInfo;
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    bufferInfo.reserve(imgCount * m_DefaultUBs.size());
    descriptorWrites.reserve(imgCount * m_DefaultUBs.size());

    for (auto&[bindingKey, uniform] : m_DefaultUBs) {
        if (uniform.IsDynamic()) {
            uniform.Allocate(instanceCount);
            for (const auto&[materialID, instanceOffset] : instanceCountScan) {
                MaterialKey materialKey(bindingKey, materialID);
                m_MaterialBufferOffsets[materialKey] = uniform.ObjectSizeAligned() * instanceOffset;
            }
        } else {
            uniform.Allocate(m_MaterialCount);
            size_t materialIdx = 0;
            for (const auto&[materialID, instanceOffset] : instanceCountScan) {
                MaterialKey materialKey(bindingKey, materialID);
                m_MaterialBufferOffsets[materialKey] = uniform.ObjectSizeAligned() * materialIdx;
                materialIdx++;
            }
        }

        /// Do not overwrite already bound buffers
        if (m_ActiveUBs.find(bindingKey) == m_ActiveUBs.end()) {
            m_ActiveUBs[bindingKey] = {true, &uniform};

            for (size_t i = 0; i < imgCount; i++) {
                bufferInfo.emplace_back(VkDescriptorBufferInfo{
                        uniform.VkHandle(),
                        uniform.BaseOffset() + (uniform.SubSize() * i),
                        uniform.ObjectSizeAligned()
                });
//            bufferInfo[idx].range = uniform.IsDynamic() ? uniform.ObjectSizeAligned() : uniform.SubSize();

                VkWriteDescriptorSet writeInfo{};
                writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeInfo.dstSet = m_DescriptorSets->get((i * layoutCount) + bindingKey.Set());
                writeInfo.dstBinding = bindingKey.Binding();
                writeInfo.dstArrayElement = 0;
                writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                writeInfo.descriptorCount = 1;
                writeInfo.pBufferInfo = &bufferInfo.back();
                descriptorWrites.push_back(writeInfo);
            }
        }
    }

    vkUpdateDescriptorSets(m_Device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}


void ShaderPipelineVk::Bind(VkCommandBuffer cmdBuffer, uint32_t imageIndex, uint32_t materialID) {
    m_CmdBuffer = cmdBuffer;
    m_ImageIndex = imageIndex;
    m_CurrentMaterialID = materialID;

    uint32_t setCount = m_DescriptorSetLayouts.size();  /// Number of layouts == stride
    uint32_t firstSet = setCount * m_ImageIndex;

    vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->data());

    for (size_t i = 0; i < m_UniformSlots.size(); i++) {
        auto ubIt = m_ActiveUBs.find(m_UniformSlots[i]);
        if (ubIt != m_ActiveUBs.end()) {
            const auto& [isDefault, ub] = ubIt->second;
            if (isDefault) {
                auto it = m_MaterialBufferOffsets.find(MaterialKey(m_UniformSlots[i], materialID));
                if (it != m_MaterialBufferOffsets.end()) {
                    m_BaseDynamicOffsets[i] = it->second;
                } else {
                    /// TODO: use logging library and replace many of those exception throws with warning messages
                    std::ostringstream msg;
                    msg << "[ShaderPipelineVk::Bind] Missing base dynamic offset for material '" << materialID << "'";
                    throw std::runtime_error(msg.str().c_str());
                }
            }
            else {
                m_BaseDynamicOffsets[i] = 0;
            }
        }
    }

    /// Bind all necessary sets for this image at once
    vkCmdBindDescriptorSets(m_CmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayout->data(),
                            0,
                            setCount,
                            &m_DescriptorSets->get(firstSet),
                            m_BaseDynamicOffsets.size(),
                            m_BaseDynamicOffsets.data());
}


void ShaderPipelineVk::SetDynamicOffsets(uint32_t set, const std::vector<uint32_t> &dynamicOffsets) const {
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


void ShaderPipelineVk::SetDynamicOffsets(uint32_t objectIndex,
                                         const std::unordered_map<BindingKey, uint32_t> &customOffsets) const {
    uint32_t setCount = m_DescriptorSetLayouts.size();  /// Number of layouts == stride
    uint32_t setIndex = setCount * m_ImageIndex;
    std::vector<uint32_t> dynamicOffsets = m_BaseDynamicOffsets;
    for (size_t i = 0; i < m_UniformSlots.size(); i++) {
        uint32_t offset = objectIndex;
        BindingKey bindingKey = m_UniformSlots[i];
        auto customOffsetIt = customOffsets.find(bindingKey);
        if (customOffsetIt != customOffsets.end()) {
            offset = customOffsetIt->second;
        }

        auto activeIt = m_ActiveUBs.find(bindingKey);
        if (activeIt != m_ActiveUBs.end()) {
            const auto& [isDefault, activeUB] = activeIt->second;
            if (activeUB->IsDynamic()) {
                dynamicOffsets[i] += activeUB->ObjectSizeAligned() * offset;
            }
        } else {
            std::ostringstream msg;
            msg << "[ShaderPipelineVk::SetDynamicOffsets] Missing uniform buffer at binding {"
                << bindingKey.Set() << ";" << bindingKey.Binding() << "}";
            throw std::runtime_error(msg.str().c_str());
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


void ShaderPipelineVk::BindUniformBuffer(const UniformBuffer *buffer, BindingKey bindingKey) {
    const auto *bufferVk = static_cast<const UniformBufferVk *>(buffer);
    auto imgCount = m_Context.Swapchain().ImageCount();
    auto layoutCount = m_DescriptorSetLayouts.size();

    /// User-bound buffers have higher precedence over default buffers
    auto activeIt = m_ActiveUBs.find(bindingKey);
    if (activeIt != m_ActiveUBs.end()) {
        const auto& [isDefault, activeUB] = activeIt->second;
        if (activeUB == bufferVk)
            return;

        activeIt->second = {false, bufferVk};
    } else {
        m_ActiveUBs[bindingKey] = {false, bufferVk};
    }

    std::vector<VkDescriptorBufferInfo> bufferInfo(imgCount);
    std::vector<VkWriteDescriptorSet> descriptorWrites(imgCount);
    for (size_t i = 0; i < imgCount; i++) {
        bufferInfo[i].buffer = bufferVk->VkHandle();
        bufferInfo[i].offset = bufferVk->BaseOffset() + (bufferVk->SubSize() * i);
        bufferInfo[i].range = bufferVk->ObjectSizeAligned();
//        bufferInfo[i].range = bufferVk.SubSize();

        descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[i].dstSet = m_DescriptorSets->get((i * layoutCount) + bindingKey.Set());
        descriptorWrites[i].dstBinding = bindingKey.Binding();
        descriptorWrites[i].dstArrayElement = 0;
        descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrites[i].descriptorCount = 1;
        descriptorWrites[i].pBufferInfo = &bufferInfo[i];
    }

    vkUpdateDescriptorSets(m_Device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

auto ShaderPipelineVk::BindTextures(const std::vector<const Texture2D *> &textures,
                                    BindingKey bindingKey) -> std::vector<uint32_t> {

    /// Update binding metadata and return continuous indices of newly bound textures
    auto it = m_BoundTextures.find(bindingKey);
    if (it == m_BoundTextures.end()) {
        it = m_BoundTextures.emplace(bindingKey, std::vector<const Texture2D *>()).first;
    }
    auto &boundTextures = it->second;
    std::vector<uint32_t> texIndices(textures.size());
    std::iota(texIndices.begin(), texIndices.end(), boundTextures.size());

    boundTextures.insert(boundTextures.end(), textures.begin(), textures.end());

    auto imgCount = m_Context.Swapchain().ImageCount();
    auto layoutCount = m_DescriptorSetLayouts.size();

    std::vector<VkDescriptorImageInfo> textureDescriptors(boundTextures.size());
    for (size_t i = 0; i < boundTextures.size(); i++) {
        const auto *textureVk = static_cast<const Texture2DVk *>(boundTextures[i]);
        textureDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        textureDescriptors[i].imageView = textureVk->View().data();
        textureDescriptors[i].sampler = m_Sampler->data();
    }

    /// TODO: doesn't work if descriptor sets numbers are not continuous and starting from 0
    /// TODO: need to do internal remapping...
    std::vector<VkWriteDescriptorSet> writeDescriptorSets(imgCount);
    for (uint32_t i = 0; i < imgCount; i++) {
        writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[i].dstSet = m_DescriptorSets->get((i * layoutCount) + bindingKey.Set());
        writeDescriptorSets[i].dstBinding = bindingKey.Binding();
        writeDescriptorSets[i].dstArrayElement = 0;
        writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[i].descriptorCount = boundTextures.size();
        writeDescriptorSets[i].pImageInfo = textureDescriptors.data();
    }

    vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

    return texIndices;
}


void ShaderPipelineVk::SetUniformData(uint32_t materialID,
                                      BindingKey bindingKey,
                                      const void *objectData, size_t objectCount) {

    auto matIt = m_BoundMaterials.find(materialID);
    if (matIt == m_BoundMaterials.end()) {
        std::ostringstream msg;
        msg << "[ShaderPipelineVk::SetUniformData] Material with ID '" << materialID
            << "' is not bound to the '" << m_Name << "' ShaderPipeline";
        throw std::runtime_error(msg.str().c_str());
    }

    MaterialKey materialKey(bindingKey, materialID);
    auto offsetIt = m_MaterialBufferOffsets.find(materialKey);
    if (offsetIt == m_MaterialBufferOffsets.end()) {
        std::ostringstream msg;
        msg << "[ShaderPipelineVk::SetUniformData] ShaderPipeline '" << m_Name
            << "' doesn't have uniform buffer binding {" << bindingKey.Set() << "," << bindingKey.Binding() << "}";
        throw std::runtime_error(msg.str().c_str());
    }

    /// TODO: do we want to write into user-bound buffers too?
    auto activeIt = m_ActiveUBs.find(bindingKey);
    if (activeIt != m_ActiveUBs.end()) {
        auto& [isDefault, activeUB] = activeIt->second;
        if (isDefault) {
            activeUB->SetData(objectData, objectCount, offsetIt->second);
        }
    }
}


auto ShaderPipelineVk::createInfo() const -> std::vector<VkPipelineShaderStageCreateInfo> {
    std::vector<VkPipelineShaderStageCreateInfo> stagesCreateInfo;
    for (const auto &it: m_ShaderModules) {
        VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
        switch (it.first) {
            case ShaderType::VERTEX_SHADER:
                shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderType::FRAGMENT_SHADER:
                shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case ShaderType::GEOMETRY_SHADER:
                shaderStage = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case ShaderType::TESSELATION_SHADER:
                shaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case ShaderType::COMPUTE_SHADER:
                shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
        }

        stagesCreateInfo.emplace_back(VkPipelineShaderStageCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr,
                0,
                shaderStage,
                it.second->data(),
                "main",
                nullptr
        });
    }

    return stagesCreateInfo;
}
