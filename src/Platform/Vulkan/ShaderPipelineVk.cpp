#include "ShaderPipelineVk.h"

#include <Engine/Renderer/Renderer.h>
#include <Engine/Application.h>
#include "RenderPassVk.h"
#include "UniformBufferVk.h"
#include "TextureVk.h"


static auto ShaderTypeToStageFlagBit(ShaderType type) -> VkShaderStageFlagBits {
   switch (type) {
      case ShaderType::VERTEX_SHADER:
         return VK_SHADER_STAGE_VERTEX_BIT;
      case ShaderType::GEOMETRY_SHADER:
         return VK_SHADER_STAGE_GEOMETRY_BIT;
      case ShaderType::TESSELATION_SHADER:
         return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
      case ShaderType::FRAGMENT_SHADER:
         return VK_SHADER_STAGE_FRAGMENT_BIT;
      case ShaderType::COMPUTE_SHADER:
         return VK_SHADER_STAGE_COMPUTE_BIT;
   }

   throw std::runtime_error("[ShaderTypeToStageFlag] Unsupported shader type");
}


ShaderPipelineVk::ShaderPipelineVk(std::string name,
                                   const std::map<ShaderType, const char *> &shaders,
                                   const std::unordered_set<BindingKey> &perObjectUniforms,
                                   const RenderPassVk &renderPass,
                                   uint32_t subpassIndex,
                                   VkPrimitiveTopology topology,
                                   std::pair<VkCullModeFlags, VkFrontFace> culling,
                                   DepthState depthState,
                                   MultisampleState msState) :
        ShaderPipeline(std::move(name)),
        m_Context(static_cast<GfxContextVk &>(Application::GetGraphicsContext())),
        m_Device(m_Context.GetDevice()),
        m_SubpassIndex(subpassIndex) {

   auto &device = static_cast<GfxContextVk &>(Application::GetGraphicsContext()).GetDevice();
   uint32_t imgCount = m_Context.Swapchain().ImageCount();

   /// TODO: expects contiguous set and binding numbers
   std::vector<std::vector<
           std::pair<VkDescriptorSetLayoutBinding, VkDescriptorBindingFlagsEXT>>> layoutDescriptions;
   std::vector<uint32_t> variableDescriptorCounts;

//    std::vector<VkPushConstantRange> ranges(m_PushRanges.size());
//    std::transform(m_PushRanges.begin(), m_PushRanges.end(), ranges.begin(), [](auto kv) { return kv.second; });

   std::vector<VkPushConstantRange> mergedStageRanges;

   std::unordered_set<BindingKey> usedBindings;
   for (const auto&[shaderType, filepath]: shaders) {
      auto *module = device.createShaderModule(filepath);
      auto shaderStageBit = ShaderTypeToStageFlagBit(shaderType);

      VkPushConstantRange mergedRange{
              shaderStageBit,
              std::numeric_limits<uint32_t>::max(),
              0,
      };

      /// Merge active ranges from all modules
      for (const auto&[index, range]: module->GetPushRanges()) {
         PushConstantKey key(shaderStageBit, index);
         mergedRange.size += range.size;
         mergedRange.offset = std::min(mergedRange.offset, range.offset);

         auto it = m_PushRanges.find(key);
         if (it != m_PushRanges.end()) {
            continue;
         } else {
            /// Check for overlaping
            VkShaderStageFlags stages = shaderStageBit;
            for (auto&[pushKey, pushRange]: m_PushRanges) {
               if (pushKey.Stage() == shaderStageBit) {
                  continue;
               }

               uint32_t lowerBound = pushRange.offset;
               uint32_t upperBound = pushRange.offset + pushRange.size;
               if ((range.offset >= lowerBound) && (range.offset < upperBound)) {
                  stages |= pushRange.stageFlags;
//                        pushRange.stageFlags = stages;
               }
            }

            m_PushRanges[key] = {
                    stages,
                    range.offset,
                    range.size
            };
         }
      }

      if (mergedRange.size > 0) {
         mergedStageRanges.push_back(mergedRange);
      }


      for (const auto&[bindingKeyValue, binding]: module->GetSamplerBindings()) {
         BindingKey bindingKey(bindingKeyValue);
         if (usedBindings.find(bindingKey) != usedBindings.end())
            continue;
         usedBindings.insert(bindingKey);

         uint32_t setIdx = bindingKey.Set();
         uint32_t bindingIdx = bindingKey.Binding();
         if (setIdx >= layoutDescriptions.size())
            layoutDescriptions.resize(setIdx + 1);

         auto &setBindings = layoutDescriptions[setIdx];
         if (bindingIdx >= setBindings.size())
            setBindings.resize(bindingIdx + 1);

         /// TODO: how to deal with arrays of textures without specified size?
         VkDescriptorBindingFlagsEXT bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

         const uint32_t DEFAULT_TEX_ARRAY_SIZE = 32;
         uint32_t descriptorCount = binding.count;
         if (binding.array && binding.count == 0) {
            descriptorCount = DEFAULT_TEX_ARRAY_SIZE;
            bindingFlags |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
                    | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
         }
         variableDescriptorCounts.push_back(descriptorCount);

         m_PoolSizes.emplace_back();
         m_PoolSizes.back().type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // TODO: allow for another types
         m_PoolSizes.back().descriptorCount = descriptorCount * imgCount;

         setBindings[bindingIdx].first.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
         setBindings[bindingIdx].first.binding = bindingIdx;
         setBindings[bindingIdx].first.descriptorCount = descriptorCount;
         setBindings[bindingIdx].first.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
         setBindings[bindingIdx].second = bindingFlags;

         m_ShaderSamplers.emplace(bindingKey, binding);
      }

      for (const auto&[bindingKeyValue, binding]: module->GetUniformBindings()) {
         BindingKey bindingKey(bindingKeyValue);
         if (usedBindings.find(bindingKey) != usedBindings.end())
            continue;
         usedBindings.insert(bindingKey);
         m_UniformBindings.insert(bindingKey);

         uint32_t setIdx = bindingKey.Set();
         uint32_t bindingIdx = bindingKey.Binding();
         if (setIdx >= layoutDescriptions.size())
            layoutDescriptions.resize(setIdx + 1);

         auto &setBindings = layoutDescriptions[setIdx];
         if (bindingIdx >= setBindings.size())
            setBindings.resize(bindingIdx + 1);

         m_PoolSizes.emplace_back();
         m_PoolSizes.back().type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
         m_PoolSizes.back().descriptorCount = binding.count * imgCount;

         variableDescriptorCounts.push_back(binding.count);

         setBindings[bindingIdx].first.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
         setBindings[bindingIdx].first.binding = bindingIdx;
         setBindings[bindingIdx].first.descriptorCount = binding.count;
         setBindings[bindingIdx].first.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
                                              | VK_SHADER_STAGE_GEOMETRY_BIT
                                              | VK_SHADER_STAGE_FRAGMENT_BIT;

         UniformBinding bindingCopy = binding;
         bindingCopy.perObject = perObjectUniforms.find(bindingKey) != perObjectUniforms.end();

         m_ShaderUniforms.emplace(bindingKey, bindingCopy);

         std::ostringstream ubName;
         ubName << "Default UB {" << bindingKey.Set() << ";" << bindingKey.Binding() << "}";
         m_DefaultUBs.emplace(
                 std::piecewise_construct,
                 std::forward_as_tuple(bindingKey),
                 std::forward_as_tuple(ubName.str(), binding.size, bindingCopy.perObject)
         );
      }
      m_ShaderModules[shaderType] = module;
   }

   if (!layoutDescriptions.empty()) {
      VkDescriptorPoolCreateFlags poolFlags = 0;
      for (const auto &bindings: layoutDescriptions) {
         auto *layout = m_Device.createDescriptorSetLayout(bindings);
         if ((layout->layoutFlags() & VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT) != 0) {
            poolFlags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
         }

         m_DescriptorSetLayouts.emplace_back(layout->data());
      }

      m_DescriptorPool = m_Device.createDescriptorPool(m_PoolSizes,
                                                       imgCount * layoutDescriptions.size(),
                                                       poolFlags);

      /// Create multiple sets for each layout to accomodate for double/triple buffering
      /// Layouts are intertwined so we can bind all sets for one image with a single vkCmdBindDescriptorSets call
      std::vector<VkDescriptorSetLayout> setCreationLayouts;
      std::vector<uint32_t> setCreationVariableDescriptorCounts;
      for (size_t imageIdx = 0; imageIdx < imgCount; imageIdx++) {
         for (const auto &setLayout: m_DescriptorSetLayouts) {
            setCreationLayouts.emplace_back(setLayout);
         }
         std::copy(variableDescriptorCounts.begin(), variableDescriptorCounts.end(),
                   std::back_inserter(setCreationVariableDescriptorCounts));
      }

      m_DescriptorSets = m_Device.createDescriptorSets(*m_DescriptorPool, setCreationLayouts,
                                                       setCreationVariableDescriptorCounts);
   }

//    std::sort(m_UsedBindings.begin(), m_UsedBindings.end(),
//              [](const BindingKey &l, const BindingKey &r) { return l.value < r.value; });

   m_BaseDynamicOffsets.resize(m_ShaderUniforms.size());

   const auto &vertexBindings = m_ShaderModules[ShaderType::VERTEX_SHADER]->GetInputBindings();
   for (const auto &bindingDescription: vertexBindings) {
      uint32_t vertexSize = 0;
      uint32_t location = 0;

      for (const auto &attribute: bindingDescription.vertexLayout) {
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


   m_Sampler = m_Device.createSampler(10.0f);
   m_Cache = m_Device.createPipelineCache(0, nullptr);

   m_PipelineLayout = m_Device.createPipelineLayout(m_DescriptorSetLayouts, mergedStageRanges);

   m_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   m_InputAssembly.topology = topology;
   m_InputAssembly.primitiveRestartEnable = VK_FALSE;

   m_Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   m_Rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Fill polygons
   m_Rasterizer.lineWidth = 1.0f; //Single fragment line width
   m_Rasterizer.cullMode = culling.first;
   m_Rasterizer.frontFace = culling.second;

   m_BlendStates = std::vector<VkPipelineColorBlendAttachmentState>(renderPass.ColorAttachmentCount(), {
           VK_FALSE,
           VK_BLEND_FACTOR_SRC_ALPHA,
           VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
           VK_BLEND_OP_ADD,
           VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
           VK_BLEND_FACTOR_ZERO,
           VK_BLEND_OP_ADD,
           VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
   });

   m_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   m_ColorBlendState.attachmentCount = m_BlendStates.size();
   m_ColorBlendState.pAttachments = m_BlendStates.data();

   m_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   m_DepthStencil.depthTestEnable = depthState.testEnable;
   m_DepthStencil.depthWriteEnable = depthState.writeEnable;
   m_DepthStencil.depthCompareOp = depthState.compareOp;
   m_DepthStencil.depthBoundsTestEnable = depthState.boundTestEnable;
   m_DepthStencil.minDepthBounds = depthState.min;
   m_DepthStencil.maxDepthBounds = depthState.max;
   m_DepthStencil.stencilTestEnable = VK_FALSE;
   m_DepthStencil.front = {}; // Optional
   m_DepthStencil.back = {}; // Optional

   m_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   m_ViewportState.viewportCount = 1;
   m_ViewportState.scissorCount = 1;

   m_Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   m_Multisampling.rasterizationSamples = msState.sampleCount;
   m_Multisampling.sampleShadingEnable = msState.sampleShadingEnable;
   m_Multisampling.minSampleShading = msState.minSampleShading;

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
   m_PipelineCreateInfo.renderPass = renderPass.data();
   m_PipelineCreateInfo.subpass = m_SubpassIndex;
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

   for (auto &m_PoolSize: m_PoolSizes)
      m_PoolSize.descriptorCount = imgCount;

   std::vector<VkDescriptorSetLayout> setCreationLayouts;
   for (size_t imageIdx = 0; imageIdx < imgCount; imageIdx++) {
      for (const auto &setLayout: m_DescriptorSetLayouts) {
         setCreationLayouts.emplace_back(setLayout);
      }
   }

   m_DescriptorPool = m_Device.createDescriptorPool(m_PoolSizes, imgCount);
   m_DescriptorSets = m_Device.createDescriptorSets(*m_DescriptorPool, setCreationLayouts, std::vector<uint32_t>());

   m_PipelineCreateInfo.renderPass = (VkRenderPass) renderPass.VkHandle();
   m_Pipeline = m_Device.createPipeline(m_PipelineCreateInfo, m_Cache->data());
}


void ShaderPipelineVk::AllocateResources() {
   uint32_t instanceCount = 0;
   std::unordered_map<uint32_t, uint32_t> instanceCountScan(m_MaterialCount);
   for (const auto&[materialID, material]: m_BoundMaterials) {
      instanceCountScan[materialID] = instanceCount;
      instanceCount += material->GetInstanceCount();
   }

   auto imgCount = m_Context.Swapchain().ImageCount();
   auto layoutCount = m_DescriptorSetLayouts.size();

   std::vector<VkDescriptorBufferInfo> bufferInfo;
   std::vector<VkWriteDescriptorSet> descriptorWrites;
   bufferInfo.reserve(imgCount * m_DefaultUBs.size());
   descriptorWrites.reserve(imgCount * m_DefaultUBs.size());

   for (auto&[bindingKey, uniform]: m_DefaultUBs) {
      if (uniform.IsDynamic()) {
         uniform.Allocate(instanceCount);
         for (const auto&[materialID, instanceOffset]: instanceCountScan) {
            MaterialKey materialKey(bindingKey, materialID);
            m_MaterialBufferOffsets[materialKey] = uniform.ObjectSizeAligned() * instanceOffset;
         }
      } else {
         uniform.Allocate(m_MaterialCount);
         size_t materialIdx = 0;
         for (const auto&[materialID, instanceOffset]: instanceCountScan) {
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


void ShaderPipelineVk::Bind(VkCommandBuffer cmdBuffer) {
   m_CmdBuffer = cmdBuffer;

   vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->data());
}


void ShaderPipelineVk::BindDescriptorSets(uint32_t imageIndex, std::optional<uint32_t> materialID) {
   m_ImageIndex = imageIndex;

   uint32_t setCount = m_DescriptorSetLayouts.size();  /// Number of layouts == stride
   uint32_t firstSet = setCount * m_ImageIndex;

   size_t uniformIdx = 0;
   for (const auto &bindingKey: m_UniformBindings) {
      auto ubIt = m_ActiveUBs.find(bindingKey);
      if (ubIt != m_ActiveUBs.end()) {
         const auto&[isDefault, ub] = ubIt->second;
         if (isDefault && materialID.has_value()) {
            auto it = m_MaterialBufferOffsets.find(MaterialKey(bindingKey, materialID.value()));
            if (it != m_MaterialBufferOffsets.end()) {
               m_BaseDynamicOffsets[uniformIdx] = it->second;
            } else {
               /// TODO: use logging library and replace many of those exception throws with warning messages
               std::ostringstream msg;
               msg << "[ShaderPipelineVk::Bind] Missing base dynamic offset for material '"
                   << materialID.value() << "'";
               throw std::runtime_error(msg.str().c_str());
            }
         } else {
            m_BaseDynamicOffsets[uniformIdx] = 0;
         }
      }
      uniformIdx++;
   }

   if (setCount > 0) {
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

   size_t uniformIdx = 0;
   for (const auto &bindingKey: m_UniformBindings) {
      uint32_t offset = objectIndex;
      auto customOffsetIt = customOffsets.find(bindingKey);
      if (customOffsetIt != customOffsets.end()) {
         offset = customOffsetIt->second;
      }

      auto activeIt = m_ActiveUBs.find(bindingKey);
      if (activeIt != m_ActiveUBs.end()) {
         const auto&[isDefault, activeUB] = activeIt->second;
         if (static_cast<const UniformBufferVk *>(activeUB)->IsDynamic()) {
            dynamicOffsets[uniformIdx] += activeUB->ObjectSizeAligned() * offset;
         }
      } else {
         std::ostringstream msg;
         msg << "[ShaderPipelineVk::SetDynamicOffsets] Missing uniform buffer at binding {"
             << bindingKey.Set() << ";" << bindingKey.Binding() << "}";
         throw std::runtime_error(msg.str().c_str());
      }
      uniformIdx++;
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
      const auto&[isDefault, activeUB] = activeIt->second;
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


auto ShaderPipelineVk::BindImageViews(const std::vector<VkImageView> &textures,
                                      BindingKey bindingKey,
                                      SamplerBinding::Type type) -> std::vector<uint32_t> {

   auto bindingIt = m_ShaderSamplers.find(bindingKey);
   if (bindingIt == m_ShaderSamplers.end()) {
      std::ostringstream msg;
      msg << "[ShaderPipelineVk::BindImageViews] Pipeline '" << m_Name
          << "' doesn't have Sampler binding (" << bindingKey.Set() << ";" << bindingKey.Binding() << ")";
      throw std::runtime_error(msg.str().c_str());
   }

   const auto &sampler = bindingIt->second;
   if (sampler.type != type) {
      std::ostringstream msg;
      msg << "[ShaderPipelineVk::BindImageViews] Pipeline '" << m_Name
          << "' SamplerBinding (" << bindingKey.Set() << ";" << bindingKey.Binding() << ") has invalid type";
      throw std::runtime_error(msg.str().c_str());
   }

   /// Update binding metadata and return continuous indices of newly bound textures
   auto it = m_BoundTextures.find(bindingKey);
   if (it == m_BoundTextures.end()) {
      it = m_BoundTextures.emplace(bindingKey, std::vector<VkImageView>()).first;
   }
   auto &boundTextures = it->second;

   if (!sampler.array && textures.size() == 1) {
      /// Just replace the previously bound texture if the sampler is not an array
      boundTextures.clear();
   }

   if (sampler.count > 0 && ((boundTextures.size() + textures.size()) > sampler.count)) {
      std::ostringstream msg;
      msg << "[ShaderPipelineVk::BindImageViews] Pipeline '" << m_Name
          << "' SamplerBinding (" << bindingKey.Set() << ";" << bindingKey.Binding() << ") can hold only '"
          << sampler.count << "' textures";
      throw std::runtime_error(msg.str().c_str());
   }

   std::vector<uint32_t> texIndices(textures.size());
   std::iota(texIndices.begin(), texIndices.end(), boundTextures.size());

   boundTextures.insert(boundTextures.end(), textures.begin(), textures.end());

   auto imgCount = m_Context.Swapchain().ImageCount();
   auto layoutCount = m_DescriptorSetLayouts.size();

   std::vector<VkDescriptorImageInfo> textureDescriptors(boundTextures.size());
   for (size_t i = 0; i < boundTextures.size(); i++) {
      textureDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      textureDescriptors[i].imageView = boundTextures[i];
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


auto ShaderPipelineVk::BindTextures2D(const std::vector<const Texture2D *> &textures,
                                      BindingKey bindingKey) -> std::vector<uint32_t> {
   std::vector<VkImageView> textureImageViews(textures.size());
   std::transform(textures.begin(), textures.end(), textureImageViews.begin(), [](const Texture2D *tex) {
      return static_cast<const Texture2DVk *>(tex)->View().data();
   });
   return BindTextures2D(textureImageViews, bindingKey);
}


auto ShaderPipelineVk::BindCubemaps(const std::vector<const TextureCubemap *> &cubemaps,
                                    BindingKey bindingKey) -> std::vector<uint32_t> {

   std::vector<VkImageView> textureImageViews(cubemaps.size());
   std::transform(cubemaps.begin(), cubemaps.end(), textureImageViews.begin(), [](const TextureCubemap *tex) {
      return static_cast<const TextureCubemapVk *>(tex)->View().data();
   });
   return BindCubemaps(textureImageViews, bindingKey);

//    /// Update binding metadata and return continuous indices of newly bound textures
//    VkImageView textureView = static_cast<const TextureCubemapVk *>(cubemaps)->View().data();
//    auto it = m_BoundTextures.find(bindingKey);
//    if (it == m_BoundTextures.end()) {
//        it = m_BoundTextures.emplace(bindingKey, std::vector<VkImageView>()).first;
//    }
//    auto &boundTextures = it->second;
//    uint32_t texIndex = boundTextures.size();
//    boundTextures.push_back(textureView);
//
//    auto imgCount = m_Context.Swapchain().ImageCount();
//    auto layoutCount = m_DescriptorSetLayouts.size();
//
//    std::vector<VkDescriptorImageInfo> textureDescriptors(boundTextures.size());
//    for (size_t i = 0; i < boundTextures.size(); i++) {
//        textureDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        textureDescriptors[i].imageView = boundTextures[i];
//        textureDescriptors[i].sampler = m_Sampler->data();
//    }
//
//    /// TODO: doesn't work if descriptor sets numbers are not continuous and starting from 0
//    /// TODO: need to do internal remapping...
//    std::vector<VkWriteDescriptorSet> writeDescriptorSets(imgCount);
//    for (uint32_t i = 0; i < imgCount; i++) {
//        writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        writeDescriptorSets[i].dstSet = m_DescriptorSets->get((i * layoutCount) + bindingKey.Set());
//        writeDescriptorSets[i].dstBinding = bindingKey.Binding();
//        writeDescriptorSets[i].dstArrayElement = 0;
//        writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        writeDescriptorSets[i].descriptorCount = boundTextures.size();
//        writeDescriptorSets[i].pImageInfo = textureDescriptors.data();
//    }
//
//    vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
//
//    return texIndex;
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
      auto&[isDefault, activeUB] = activeIt->second;
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

