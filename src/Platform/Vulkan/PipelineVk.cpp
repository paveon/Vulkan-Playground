#include "PipelineVk.h"

#include <Engine/Application.h>
#include "RenderPassVk.h"
#include "ShaderProgramVk.h"


static VkFormat ShaderTypeToVkFormat(ShaderType type) {
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


static VkDescriptorType DescriptorTypeToVk(DescriptorType type) {
   switch (type) {
      case DescriptorType::Texture:
         return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      case DescriptorType::UniformBuffer:
         return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   }

   throw std::runtime_error("Unknown descriptor type");
}


PipelineVk::PipelineVk(const RenderPassVk& renderPass, const ShaderProgramVk& vertexShader,
                       const ShaderProgramVk& fragShader, const VertexLayout& vertexLayout,
                       const DescriptorLayout& descriptorLayout, const std::vector<PushConstant>& pushConstants,
                       bool enableDepthTest) {

   auto& gfxContext = static_cast<GfxContextVk&>(Application::GetGraphicsContext());
   const Device& device = gfxContext.GetDevice();
   auto imgCount = gfxContext.Swapchain().ImageCount();

   m_Sampler = vk::Sampler(device, 0.0);

   std::vector<VkDescriptorPoolSize> poolSizes;
   std::vector<VkDescriptorSetLayoutBinding> bindings;
   for (size_t bindingIdx = 0; bindingIdx < descriptorLayout.size(); bindingIdx++) {
      VkDescriptorType type = DescriptorTypeToVk(descriptorLayout[bindingIdx].type);

      poolSizes.emplace_back();
      poolSizes.back().type = type;
      poolSizes.back().descriptorCount = imgCount;

      bindings.emplace_back();
      VkDescriptorSetLayoutBinding& binding = bindings.back();
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
            break;
      }
   }

   m_DescriptorPool = device.createDescriptorPool(poolSizes, imgCount);
   m_DescriptorSetLayout = device.createDescriptorSetLayout(bindings);

   std::vector<VkDescriptorSetLayout> layouts(imgCount, m_DescriptorSetLayout.data());
   m_DescriptorSets = vk::DescriptorSets(device, m_DescriptorPool.data(), layouts);

   m_Cache = vk::PipelineCache(device, 0, nullptr);

   uint32_t currentOffset = 0;
   std::vector<VkPushConstantRange> ranges(pushConstants.size());
   for (size_t i = 0; i < ranges.size(); i++) {
      ranges[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      ranges[i].size = pushConstants[i].size;
      ranges[i].offset = currentOffset;
      currentOffset += pushConstants[i].size;
   }

   m_PipelineLayout = vk::PipelineLayout(device, {m_DescriptorSetLayout.data()}, {ranges});

   VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
   inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   inputAssembly.primitiveRestartEnable = VK_FALSE;

   VkPipelineRasterizationStateCreateInfo rasterizer = {};
   rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Fill polygons
   rasterizer.lineWidth = 1.0f; //Single fragment line width
   rasterizer.cullMode = VK_CULL_MODE_NONE; //Back-face culling
   rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //Front-facing polygons have vertices in clockwise order

   VkPipelineColorBlendAttachmentState blendAttachmentState{};
   blendAttachmentState.blendEnable = VK_TRUE;
   blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                         VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT |
                                         VK_COLOR_COMPONENT_A_BIT;
   blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
   blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

   VkPipelineColorBlendStateCreateInfo colorBlendState = {};
   colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlendState.attachmentCount = 1;
   colorBlendState.pAttachments = &blendAttachmentState;

   VkPipelineDepthStencilStateCreateInfo depthStencil = {};
   if (enableDepthTest) {
      depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      depthStencil.depthTestEnable = VK_TRUE;
      depthStencil.depthWriteEnable = VK_TRUE;
      depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
      depthStencil.depthBoundsTestEnable = VK_FALSE;
      depthStencil.minDepthBounds = 0.0f; // Optional
      depthStencil.maxDepthBounds = 1.0f; // Optional
      depthStencil.stencilTestEnable = VK_FALSE;
      depthStencil.front = {}; // Optional
      depthStencil.back = {}; // Optional
   }
   else {
      depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      depthStencil.depthTestEnable = VK_FALSE;
      depthStencil.depthWriteEnable = VK_FALSE;
      depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      depthStencil.front = depthStencil.back;
      depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
   }


   VkPipelineViewportStateCreateInfo viewportState = {};
   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportState.viewportCount = 1;
   viewportState.scissorCount = 1;

   VkPipelineMultisampleStateCreateInfo multisampling = {};
   multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; //TODO

   std::array<VkDynamicState, 2> dynamicStateEnables = {
           VK_DYNAMIC_STATE_VIEWPORT,
           VK_DYNAMIC_STATE_SCISSOR
   };

   VkPipelineDynamicStateCreateInfo dynamicState = {};
   dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicState.dynamicStateCount = dynamicStateEnables.size();
   dynamicState.pDynamicStates = dynamicStateEnables.data();

   uint32_t vertexSize = 0;
   uint32_t location = 0;
   std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
   for (const VertexAttribute& attribute : vertexLayout) {
      VkVertexInputAttributeDescription inputAttribute = {};
      inputAttribute.location = location++;
      inputAttribute.binding = 0; //TODO;
      inputAttribute.format = ShaderTypeToVkFormat(attribute.format);
      inputAttribute.offset = vertexSize;
      vertexInputAttributes.push_back(inputAttribute);
      vertexSize += ShaderTypeSize(attribute.format);
   }

   VkVertexInputBindingDescription vInputBindDescription = {};
   vInputBindDescription.binding = 0;
   vInputBindDescription.stride = vertexSize;
   vInputBindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

   VkPipelineVertexInputStateCreateInfo vertexInputState = {};
   vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputState.vertexBindingDescriptionCount = 1;
   vertexInputState.pVertexBindingDescriptions = &vInputBindDescription;
   vertexInputState.vertexAttributeDescriptionCount = vertexInputAttributes.size();
   vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

   std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{
           vertexShader.createInfo(VK_SHADER_STAGE_VERTEX_BIT),
           fragShader.createInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
   };

   VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
   pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineCreateInfo.layout = m_PipelineLayout.data();
   pipelineCreateInfo.renderPass = renderPass.data();
   pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
   pipelineCreateInfo.pRasterizationState = &rasterizer;
   pipelineCreateInfo.pColorBlendState = &colorBlendState;
   pipelineCreateInfo.pMultisampleState = &multisampling;
   pipelineCreateInfo.pViewportState = &viewportState;
   pipelineCreateInfo.pDepthStencilState = &depthStencil;
   pipelineCreateInfo.pDynamicState = &dynamicState;
   pipelineCreateInfo.stageCount = shaderStages.size();
   pipelineCreateInfo.pStages = shaderStages.data();
   pipelineCreateInfo.pVertexInputState = &vertexInputState;

   m_Pipeline = vk::Pipeline(device, pipelineCreateInfo, m_Cache.data());
}


void PipelineVk::Bind(VkCommandBuffer cmdBuffer, uint32_t frameIdx) const {
   vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           m_PipelineLayout.data(), 0, 1, &m_DescriptorSets[frameIdx], 0, nullptr);

   vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.data());
}