#define GLFW_INCLUDE_VULKAN

//#define MTH_DEPTH_NEGATIVE_TO_POSITIVE_ONE

#include <iostream>
#include <cstring>
#include <cassert>
#include <vector>
#include <set>

#include <GLFW/glfw3.h>
#include <mathlib.h>
#include <Platform/Vulkan/TextureVk.h>
#include <Platform/Vulkan/ShaderProgramVk.h>

#include "utils.h"
#include "renderer.h"

const char* MODEL_PATH = BASE_DIR "/models/chalet.obj";
const char* TEXTURE_PATH = BASE_DIR "/textures/chalet.jpg";


GraphicsAPI Renderer::s_API = GraphicsAPI::VULKAN;


Renderer::Renderer(GfxContextVk& ctx) : m_Context(ctx), m_Device(ctx.GetDevice()), m_Swapchain(ctx.Swapchain()) {
   m_RenderPass = RenderPass::Create();

   std::unique_ptr<ShaderProgram> fragShader(ShaderProgram::Create(fragShaderPath));
   std::unique_ptr<ShaderProgram> vertexShader(ShaderProgram::Create(vertShaderPath));
   VertexLayout vertexLayout{
           VertexAttribute(ShaderType::Float3),
           VertexAttribute(ShaderType::Float3),
           VertexAttribute(ShaderType::Float2)
   };

   DescriptorLayout descriptorLayout{
           DescriptorBinding(DescriptorType::UniformBuffer),
           DescriptorBinding(DescriptorType::Texture)
   };

   m_GraphicsPipeline = Pipeline::Create(*m_RenderPass, *vertexShader, *fragShader, vertexLayout, descriptorLayout, {}, true);

   m_GraphicsCmdPool = m_Device.createCommandPool(m_Device.queueIndex(QueueFamily::GRAPHICS),
                                                  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
   m_TransferCmdPool = m_Device.createCommandPool(m_Device.queueIndex(QueueFamily::TRANSFER),
                                                  VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

   std::set<uint32_t> queueIndices{
           m_Device.queueIndex(QueueFamily::TRANSFER),
           m_Device.queueIndex(QueueFamily::GRAPHICS),
   };

   /* Create texture resources */
   m_Texture = Texture2D::Create(TEXTURE_PATH);
   m_Texture->Upload();

   m_ColorImage = m_Device.createImage({m_Device.queueIndex(QueueFamily::GRAPHICS)}, m_Swapchain.Extent(), 1,
                                       m_Device.maxSamples(),
                                       m_Swapchain.ImageFormat(), VK_IMAGE_TILING_OPTIMAL,
                                       VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
   m_ColorImageMemory = m_Device.allocateImageMemory(m_ColorImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   vkBindImageMemory(m_Device, m_ColorImage.data(), m_ColorImageMemory.data(), 0);
   m_ColorImageView = m_ColorImage.createView(m_Swapchain.ImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT);

   //StagingBuffer textureBuffer(m_Device, texture->Data(), texture->Size());
   vk::CommandBuffer setupCmdBuffer(m_Device, m_TransferCmdPool.data());
   setupCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

   /* Transition texture image */
   //m_TextureImage.ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
   //copyBufferToImage(setupCmdBuffer, textureBuffer, m_TextureImage);

   /* Create vertex and index buffers */
   m_Model = std::make_unique<Model>(MODEL_PATH);
   m_VertexBuffer = DeviceBuffer(m_Device, m_Model->Vertices(), m_Model->VertexDataSize(), setupCmdBuffer,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
   m_IndexBuffer = DeviceBuffer(m_Device, m_Model->Indices(), m_Model->IndexDataSize(), setupCmdBuffer,
                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

   /* Transfer queue ownership of resources */
   std::array<VkBufferMemoryBarrier, 2> bufferBarriers = {};
   bufferBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
   bufferBarriers[0].srcQueueFamilyIndex = m_Device.queueIndex(QueueFamily::TRANSFER);
   bufferBarriers[0].dstQueueFamilyIndex = m_Device.queueIndex(QueueFamily::GRAPHICS);
   bufferBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
   bufferBarriers[0].dstAccessMask = 0;
   bufferBarriers[0].buffer = m_VertexBuffer.data().data();
   bufferBarriers[0].size = m_VertexBuffer.size();
   bufferBarriers[0].offset = 0;
   bufferBarriers[1] = bufferBarriers[0];
   bufferBarriers[1].buffer = m_IndexBuffer.data().data();
   bufferBarriers[1].size = m_IndexBuffer.size();

//   std::array<VkImageMemoryBarrier, 1> imageBarriers = {};
//   imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//   //imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//   //imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//   imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//   imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//   imageBarriers[0].srcQueueFamilyIndex = m_Device.queueIndex(QueueFamily::TRANSFER);
//   imageBarriers[0].dstQueueFamilyIndex = m_Device.queueIndex(QueueFamily::GRAPHICS);
//   imageBarriers[0].image = m_TextureImage.data();
//   imageBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//   imageBarriers[0].subresourceRange.baseMipLevel = 0;
//   imageBarriers[0].subresourceRange.levelCount = m_TextureImage.MipLevels();
//   imageBarriers[0].subresourceRange.baseArrayLayer = 0;
//   imageBarriers[0].subresourceRange.layerCount = 1;
//   imageBarriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//   imageBarriers[0].dstAccessMask = 0;

   vkCmdPipelineBarrier(setupCmdBuffer.data(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
                        0, nullptr,
                        bufferBarriers.size(), bufferBarriers.data(),
                        0, nullptr
   );

   setupCmdBuffer.End();

   vk::Semaphore transferSemaphore(m_Device);
   VkSubmitInfo submitInfo = {};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.signalSemaphoreCount = 1;
   submitInfo.pSignalSemaphores = transferSemaphore.ptr();
   setupCmdBuffer.Submit(submitInfo, m_Device.queue(QueueFamily::TRANSFER));

   vk::CommandBuffer acquisitionCmdBuffer = m_Device.createCommandBuffer(m_GraphicsCmdPool);
   acquisitionCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

   m_ColorImage.ChangeLayout(acquisitionCmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

   bufferBarriers[0].srcAccessMask = 0;
   bufferBarriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
   bufferBarriers[1].srcAccessMask = 0;
   bufferBarriers[1].dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
   //imageBarriers[0].srcAccessMask = 0;
   //imageBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
   vkCmdPipelineBarrier(acquisitionCmdBuffer.data(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
                        0, nullptr,
                        bufferBarriers.size(), bufferBarriers.data(),
                        0, nullptr
   );

//   vkCmdPipelineBarrier(acquisitionCmdBuffer.data(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
//                        0,
//                        0, nullptr,
//                        0, nullptr,
//                        imageBarriers.size(), imageBarriers.data()
//   );


   //m_TextureImage.GenerateMipmaps(m_Device, acquisitionCmdBuffer);

   acquisitionCmdBuffer.End();

   VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores = transferSemaphore.ptr();
   submitInfo.pWaitDstStageMask = &waitStage;
   acquisitionCmdBuffer.Submit(submitInfo, m_Device.queue(QueueFamily::GRAPHICS));

   vkQueueWaitIdle(m_Device.queue(QueueFamily::GRAPHICS));


   std::vector<VkImageView> attachments{nullptr, m_DepthImageView.data()};
   if (m_Device.maxSamples() != VK_SAMPLE_COUNT_1_BIT) {
      attachments.push_back(m_ColorImageView.data());
   }

   for (const vk::ImageView& view : m_Swapchain.ImageViews()) {
      attachments.front() = view.data();
      m_SwapchainFramebuffers.emplace_back(
              vk::Framebuffer(m_Device, m_RenderPass->GetVkHandle(), m_Swapchain.Extent(), attachments));
   }

   createUniformBuffers(m_Swapchain.ImageCount());
   m_GraphicsPipeline->BindUniformBuffers(m_UniformBuffers, 0);
   m_GraphicsPipeline->BindTexture(*m_Texture, 1);

   //updateDescriptorSets(m_DescriptorSets);
   m_GraphicsCmdBuffers = m_Device.createCommandBuffers(m_GraphicsCmdPool, m_SwapchainFramebuffers.size());
   //recordCommandBuffers(m_GraphicsCmdBuffers);
   createSyncObjects();

   //prepareImGui(m_Swapchain.Extent(), m_RenderPass, m_Device.queue(QueueFamily::GRAPHICS));
}


Renderer::~Renderer() {
   vkDeviceWaitIdle(m_Device);
}


vk::Pipeline Renderer::createGraphicsPipeline(VkPipelineLayout layout, VkRenderPass renderPass) {
   auto bindingDescription = Vertex::getBindingDescription();
   auto attributeDescriptions = Vertex::getAttributeDescriptions();

   VkPipelineVertexInputStateCreateInfo vertexInputState = {};
   vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputState.vertexBindingDescriptionCount = 1;
   vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
   vertexInputState.pVertexBindingDescriptions = &bindingDescription;
   vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

   VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
   inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   inputAssemblyState.primitiveRestartEnable = VK_FALSE;

   VkViewport viewport = {};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(m_Swapchain.Extent().width);
   viewport.height = static_cast<float>(m_Swapchain.Extent().height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor = {};
   scissor.offset = {0, 0};
   scissor.extent = m_Swapchain.Extent();

   VkPipelineViewportStateCreateInfo viewportState = {};
   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportState.viewportCount = 1;
   viewportState.pViewports = &viewport;
   viewportState.scissorCount = 1;
   viewportState.pScissors = &scissor;

   VkPipelineRasterizationStateCreateInfo rasterizer = {};
   rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.depthClampEnable = VK_FALSE; //Discard fragments beyond near/far planes instead of clamping
   rasterizer.rasterizerDiscardEnable = VK_FALSE; //Do not discard fragments
   rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Fill polygons
   rasterizer.lineWidth = 1.0f; //Single fragment line width
   rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //Back-face culling
   rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //Front-facing polygons have vertices in clockwise order
   rasterizer.depthBiasEnable = VK_FALSE;
   rasterizer.depthBiasConstantFactor = 0.0f; // Optional
   rasterizer.depthBiasClamp = 0.0f; // Optional
   rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

   VkPipelineMultisampleStateCreateInfo multisampleState = {};
   multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampleState.sampleShadingEnable = VK_TRUE;
   multisampleState.minSampleShading = 0.4f; // Optional
   multisampleState.rasterizationSamples = m_Device.maxSamples();
   multisampleState.pSampleMask = nullptr; // Optional
   multisampleState.alphaToCoverageEnable = VK_FALSE; // Optional
   multisampleState.alphaToOneEnable = VK_FALSE; // Optional

   VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
   colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                         VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT |
                                         VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable = VK_FALSE;
   colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
   colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
   colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
   colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
   colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
   colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

   //Alpha blending
//      colorBlendAttachment.blendEnable = VK_TRUE;
//      colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//      colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//      colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
//      colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//      colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//      colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

   VkPipelineColorBlendStateCreateInfo blendState = {};
   blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   blendState.logicOpEnable = VK_FALSE;
   blendState.logicOp = VK_LOGIC_OP_COPY; // Optional
   blendState.attachmentCount = 1;
   blendState.pAttachments = &colorBlendAttachment;
   blendState.blendConstants[0] = 0.0f; // Optional
   blendState.blendConstants[1] = 0.0f; // Optional
   blendState.blendConstants[2] = 0.0f; // Optional
   blendState.blendConstants[3] = 0.0f; //

   std::array<VkDynamicState, 2> dynamicStates{
           VK_DYNAMIC_STATE_VIEWPORT,
           VK_DYNAMIC_STATE_SCISSOR
   };

   VkPipelineDynamicStateCreateInfo dynamicState = {};
   dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicState.dynamicStateCount = dynamicStates.size();
   dynamicState.pDynamicStates = dynamicStates.data();

   VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
   depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
   depthStencilState.depthTestEnable = VK_TRUE;
   depthStencilState.depthWriteEnable = VK_TRUE;
   depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
   depthStencilState.depthBoundsTestEnable = VK_FALSE;
   depthStencilState.minDepthBounds = 0.0f; // Optional
   depthStencilState.maxDepthBounds = 1.0f; // Optional
   depthStencilState.stencilTestEnable = VK_FALSE;
   depthStencilState.front = {}; // Optional
   depthStencilState.back = {}; // Optional

   ShaderProgramVk vertShader(vertShaderPath);
   ShaderProgramVk fragShader(fragShaderPath);

   std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
           vertShader.createInfo(VK_SHADER_STAGE_VERTEX_BIT),
           fragShader.createInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
   };

   VkGraphicsPipelineCreateInfo pipelineInfo = {};
   pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.stageCount = shaderStages.size();
   pipelineInfo.pStages = shaderStages.data();
   pipelineInfo.pVertexInputState = &vertexInputState;
   pipelineInfo.pInputAssemblyState = &inputAssemblyState;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampleState;
   pipelineInfo.pDepthStencilState = &depthStencilState;
   pipelineInfo.pColorBlendState = &blendState;
   pipelineInfo.pDynamicState = &dynamicState;
   pipelineInfo.layout = layout;
   pipelineInfo.renderPass = renderPass;
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
   pipelineInfo.basePipelineIndex = -1; // Optional

//   VkPipeline pipeline;
//   if (vkCreateGraphicsPipelines(m_Device, nullptr, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
//      throw std::runtime_error("failed to create graphics pipeline!");
//   }

   return vk::Pipeline(m_Device, pipelineInfo);
}


VkRenderPass Renderer::createRenderPass() {
   VkSubpassDescription subpass = {};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;

   std::vector<VkAttachmentDescription> attachments{{},
                                                    {}};

   VkAttachmentDescription& colorAttachment(attachments[0]);
   colorAttachment.format = m_Swapchain.ImageFormat();
   colorAttachment.samples = m_Device.maxSamples();
   colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   VkAttachmentDescription& depthAttachment(attachments[1]);
   depthAttachment.format = m_Device.findDepthFormat();
   depthAttachment.samples = m_Device.maxSamples();
   depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   VkAttachmentReference colorAttachmentRef = {};
   colorAttachmentRef.attachment = 0;
   colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkAttachmentReference depthAttachmentRef = {};
   depthAttachmentRef.attachment = 1;
   depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

   /* Add resolve attachment if multisampling is enabled */
   VkAttachmentReference colorAttachmentResolveRef = {};
   if (m_Device.maxSamples() != VK_SAMPLE_COUNT_1_BIT) {
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentDescription colorAttachmentResolve = {};
      colorAttachmentResolve.format = m_Swapchain.ImageFormat();
      colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      colorAttachmentResolveRef.attachment = 2;
      colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      subpass.pResolveAttachments = &colorAttachmentResolveRef;

      attachments.push_back(colorAttachmentResolve);
   }

   subpass.pColorAttachments = &colorAttachmentRef;
   subpass.pDepthStencilAttachment = &depthAttachmentRef;

   VkSubpassDependency dependency = {};
   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass = 0;
   dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.srcAccessMask = 0;
   dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.dstAccessMask =
           VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

   VkRenderPassCreateInfo renderPassInfo = {};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = attachments.size();
   renderPassInfo.pAttachments = attachments.data();
   renderPassInfo.subpassCount = 1;
   renderPassInfo.pSubpasses = &subpass;
   renderPassInfo.dependencyCount = 1;
   renderPassInfo.pDependencies = &dependency;

   VkRenderPass renderPass;
   if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
      throw std::runtime_error("failed to create render pass!");
   }

   return renderPass;
}


void Renderer::recordCommandBuffers(const vk::CommandBuffers& cmdBuffers) {
   VkViewport viewport = {};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(m_Swapchain.Extent().width);
   viewport.height = static_cast<float>(m_Swapchain.Extent().height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor = {};
   scissor.offset = {0, 0};
   scissor.extent = m_Swapchain.Extent();

   VkCommandBufferBeginInfo beginInfo = {};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
   beginInfo.pInheritanceInfo = nullptr; // Optional

   VkClearColorValue colorClear = {{0.0f, 0.0f, 0.0f, 1.0f}};
   std::array<VkClearValue, 2> clearValues = {};
   clearValues[0].color = colorClear;
   clearValues[1].depthStencil = {1.0f, 0};

   VkRenderPassBeginInfo renderPassInfo = {};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassInfo.renderPass = m_RenderPass->GetVkHandle();
   renderPassInfo.renderArea.offset = {0, 0};
   renderPassInfo.renderArea.extent = m_Swapchain.Extent();
   renderPassInfo.clearValueCount = clearValues.size();
   renderPassInfo.pClearValues = clearValues.data();

   for (size_t i = 0; i < cmdBuffers.size(); i++) {
      if (vkBeginCommandBuffer(cmdBuffers[i], &beginInfo) != VK_SUCCESS) {
         throw std::runtime_error("failed to begin recording command buffer!");
      }

      VkImageMemoryBarrier renderBeginBarrier = imageBarrier(m_Swapchain.Images()[i], 0,
                                                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                             VK_IMAGE_LAYOUT_UNDEFINED,
                                                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

      vkCmdPipelineBarrier(cmdBuffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_DEPENDENCY_BY_REGION_BIT,
                           0, nullptr, 0, nullptr, 1, &renderBeginBarrier);

      vkCmdSetViewport(cmdBuffers[i], 0, 1, &viewport);
      vkCmdSetScissor(cmdBuffers[i], 0, 1, &scissor);

      renderPassInfo.framebuffer = m_SwapchainFramebuffers[i].data();

      vkCmdBeginRenderPass(cmdBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

      m_GraphicsPipeline->Bind(cmdBuffers[i], i);
      //vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.data());

      VkDeviceSize bufferOffset = 0;
      vkCmdBindVertexBuffers(cmdBuffers[i], 0, 1, m_VertexBuffer.data().ptr(), &bufferOffset);

      vkCmdBindIndexBuffer(cmdBuffers[i], m_IndexBuffer.data().data(), 0, VK_INDEX_TYPE_UINT32);

      //vkCmdBindDescriptorSets(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout.data(), 0, 1, &m_DescriptorSets[i], 0, nullptr);

      vkCmdDrawIndexed(cmdBuffers[i], m_Model->IndexCount(), 1, 0, 0, 0);

      vkCmdEndRenderPass(cmdBuffers[i]);


//      VkImageMemoryBarrier renderEndBarrier = imageBarrier(m_SwapchainImages[i],
//                                                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
//                                                           0,
//                                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//                                                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
//
//      vkCmdPipelineBarrier(cmdBuffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//                           VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT,
//                           0, nullptr,
//                           0, nullptr,
//                           1, &renderEndBarrier);

      if (vkEndCommandBuffer(cmdBuffers[i]) != VK_SUCCESS) {
         throw std::runtime_error("failed to record command buffer!");
      }
   }
}


void Renderer::recordCommandBuffer(VkCommandBuffer cmdBuffer, size_t index) {
   VkViewport viewport = {};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(m_Swapchain.Extent().width);
   viewport.height = static_cast<float>(m_Swapchain.Extent().height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor = {};
   scissor.offset = {0, 0};
   scissor.extent = m_Swapchain.Extent();

   VkCommandBufferBeginInfo beginInfo = {};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
   beginInfo.pInheritanceInfo = nullptr; // Optional

   VkClearColorValue colorClear = {{0.0f, 0.0f, 0.0f, 1.0f}};
   std::array<VkClearValue, 2> clearValues = {};
   clearValues[0].color = colorClear;
   clearValues[1].depthStencil = {1.0f, 0};

   VkRenderPassBeginInfo renderPassBeginInfo = {};
   renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassBeginInfo.renderPass = m_RenderPass->GetVkHandle();
   renderPassBeginInfo.renderArea.offset = {0, 0};
   renderPassBeginInfo.renderArea.extent = m_Swapchain.Extent();
   renderPassBeginInfo.clearValueCount = clearValues.size();
   renderPassBeginInfo.pClearValues = clearValues.data();
   renderPassBeginInfo.framebuffer = m_SwapchainFramebuffers[index].data();

   //TODO
   //m_ImGui->newFrame(frameTimer, false);

   //m_ImGui->updateBuffers();

   if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
   }

   VkImageMemoryBarrier renderBeginBarrier = imageBarrier(m_Swapchain.Images()[index], 0,
                                                          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                          VK_IMAGE_LAYOUT_UNDEFINED,
                                                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

   vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_DEPENDENCY_BY_REGION_BIT,
                        0, nullptr, 0, nullptr, 1, &renderBeginBarrier);

   vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

   vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
   vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

   //vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout.data(), 0, 1, &m_DescriptorSets[index], 0, nullptr);

   m_GraphicsPipeline->Bind(cmdBuffer, index);
   //vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.data());

   VkDeviceSize bufferOffset = 0;
   vkCmdBindVertexBuffers(cmdBuffer, 0, 1, m_VertexBuffer.data().ptr(), &bufferOffset);

   vkCmdBindIndexBuffer(cmdBuffer, m_IndexBuffer.data().data(), 0, VK_INDEX_TYPE_UINT32);

   vkCmdDrawIndexed(cmdBuffer, m_Model->IndexCount(), 1, 0, 0, 0);

   //m_ImGui->drawFrame(cmdBuffer);

   vkCmdEndRenderPass(cmdBuffer);

   if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
   }
}


void Renderer::createSyncObjects() {
   for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      m_AcquireSemaphores.emplace_back(m_Device.createSemaphore());
      m_ReleaseSemaphores.emplace_back(m_Device.createSemaphore());
      m_Fences.emplace_back(m_Device.createFence(true));
   }
}


void Renderer::createUniformBuffers(size_t count) {
   VkDeviceSize bufferSize = sizeof(UniformBufferObject);

   VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

   m_UniformBuffers.reserve(count);
   m_UniformMemory.reserve(count);
   for (size_t i = 0; i < count; i++) {
      m_UniformBuffers.emplace_back(m_Device.createBuffer({m_Device.queueIndex(QueueFamily::GRAPHICS)}, bufferSize,
                                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));

      m_UniformMemory.emplace_back(m_Device.allocateBufferMemory(m_UniformBuffers.back(), memoryFlags));
      vkBindBufferMemory(m_Device, m_UniformBuffers.back().data(), m_UniformMemory.back().data(), 0);
   }
}


//void Renderer::updateDescriptorSets(const vk::DescriptorSets& descriptorSets) {
//   for (size_t i = 0; i < descriptorSets.size(); i++) {
//      VkDescriptorBufferInfo bufferInfo = {};
//      bufferInfo.buffer = m_UniformBuffers[i].data();
//      bufferInfo.offset = 0;
//      bufferInfo.range = sizeof(UniformBufferObject);
//
//      VkDescriptorImageInfo imageInfo = {};
//      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//      imageInfo.imageView = m_TextureImageView.data();
//      imageInfo.sampler = m_TextureSampler.data();
//
//      std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
//      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//      descriptorWrites[0].dstSet = descriptorSets[i];
//      descriptorWrites[0].dstBinding = 0;
//      descriptorWrites[0].dstArrayElement = 0;
//      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//      descriptorWrites[0].descriptorCount = 1;
//      descriptorWrites[0].pBufferInfo = &bufferInfo;
//
//      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//      descriptorWrites[1].dstSet = descriptorSets[i];
//      descriptorWrites[1].dstBinding = 1;
//      descriptorWrites[1].dstArrayElement = 0;
//      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//      descriptorWrites[1].descriptorCount = 1;
//      descriptorWrites[1].pImageInfo = &imageInfo;
//
//      vkUpdateDescriptorSets(m_Device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
//   }
//}


void Renderer::cleanupSwapchain() {
   //vkDestroyPipeline(Device, m_GraphicsPipeline, nullptr);
   //vkDestroyPipelineLayout(Device, m_PipelineLayout, nullptr);

   m_SwapchainFramebuffers.clear();
   m_Swapchain.Release();

   if (m_RenderPass) m_RenderPass.reset();
}


void Renderer::RecreateSwapchain(uint32_t width, uint32_t height) {
//   vkDeviceWaitIdle(m_Device);
//
//   m_FramebufferResized = true;
//
//   cleanupSwapchain();
//
//   auto caps = m_Device.getSurfaceCapabilities();
//   VkExtent2D newExtent = chooseSwapExtent({width, height}, caps);
//   m_Swapchain = m_Device.createSwapChain({
//                                                  m_Device.queueIndex(QueueFamily::GRAPHICS),
//                                                  m_Device.queueIndex(QueueFamily::PRESENT)},
//                                          newExtent);
//
//   m_RenderPass = RenderPass::Create();
//   //m_RenderPass = createRenderPass();
//
//   VkFormat depthFormat = m_Device.findDepthFormat();
//   m_DepthImage = m_Device.createImage({m_Device.queueIndex(QueueFamily::GRAPHICS)}, m_Swapchain.Extent(), 1,
//                                       m_Device.maxSamples(), depthFormat,
//                                       VK_IMAGE_TILING_OPTIMAL,
//                                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
//   m_DepthImageMemory = m_Device.allocateImageMemory(m_DepthImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//   vkBindImageMemory(m_Device, m_DepthImage.data(), m_DepthImageMemory.data(), 0);
//   m_DepthImageView = m_DepthImage.createView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
//
//   m_ColorImage = m_Device.createImage({m_Device.queueIndex(QueueFamily::GRAPHICS)}, m_Swapchain.Extent(), 1,
//                                       m_Device.maxSamples(),
//                                       m_Swapchain.ImageFormat(), VK_IMAGE_TILING_OPTIMAL,
//                                       VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
//   m_ColorImageMemory = m_Device.allocateImageMemory(m_ColorImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//   vkBindImageMemory(m_Device, m_ColorImage.data(), m_ColorImageMemory.data(), 0);
//   m_ColorImageView = m_ColorImage.createView(m_Swapchain.ImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT);
//
//   vk::CommandBuffer cmdBuffer(m_Device, m_GraphicsCmdPool.data());
//   cmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
//   m_ColorImage.ChangeLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//   m_DepthImage.ChangeLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
//   cmdBuffer.End();
//   cmdBuffer.Submit(m_Device.queue(QueueFamily::GRAPHICS));
//
//   //createGraphicsPipeline();
//
//   std::vector<VkImageView> attachments{nullptr, m_DepthImageView.data()};
//   if (m_Device.maxSamples() != VK_SAMPLE_COUNT_1_BIT) {
//      attachments.push_back(m_ColorImageView.data());
//   }
//
//   for (const vk::ImageView& view : m_Swapchain.ImageViews()) {
//      attachments.front() = view.data();
//      m_SwapchainFramebuffers.emplace_back(
//              vk::Framebuffer(m_Device, m_RenderPass->GetVkHandle(), m_Swapchain.Extent(), attachments));
//   }
//
//   createUniformBuffers(m_Swapchain.Images().size());
//
//   std::vector<VkDescriptorPoolSize> poolSizes(2);
//   poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//   poolSizes[0].descriptorCount = m_Swapchain.ImageCount();
//   poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//   poolSizes[1].descriptorCount = m_Swapchain.ImageCount();
//   m_DescriptorPool = m_Device.createDescriptorPool(poolSizes, m_Swapchain.Images().size());
//
//   std::vector<VkDescriptorSetLayout> clonedDescriptors(m_Swapchain.Images().size(), m_DescriptorSetLayout.data());
//   m_DescriptorSets = m_Device.createDescriptorSets(m_DescriptorPool, clonedDescriptors);
//
//   updateDescriptorSets(m_DescriptorSets);
//   m_GraphicsCmdBuffers = m_Device.createCommandBuffers(m_GraphicsCmdPool, m_SwapchainFramebuffers.size());
//   //recordCommandBuffers(m_GraphicsCmdBuffers);
//
//   vkQueueWaitIdle(m_Device.queue(QueueFamily::GRAPHICS));
//
//   m_FramebufferResized = false;
}


void Renderer::updateUniformBuffer(uint32_t currentImage) {
   static auto startTime = TIME_NOW;

   auto currentTime = TIME_NOW;
   float time = std::chrono::duration<float, std::chrono::seconds::period>(
           currentTime - startTime).count();

   UniformBufferObject ubo = {};
   ubo.model = math::rotate(ubo.model, time * math::radians(15.0f), math::vec3(0.0f, 0.0f, 1.0f));

   math::vec3 cameraPos(2.0f, 2.0f, 2.0f);
   math::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
   math::vec3 upVector(0.0f, 0.0f, 1.0f);
   ubo.view = math::lookAt(cameraPos, cameraTarget, upVector);

   float verticalFov = math::radians(45.0f);
   float aspectRatio = m_Swapchain.Extent().width / (float) m_Swapchain.Extent().height;
   float nearPlane = 0.1f;
   float farPlane = 10.0f;
   ubo.proj = math::perspective(verticalFov, aspectRatio, nearPlane, farPlane);
   ubo.proj[1][1] *= -1;

   void* data;
   vkMapMemory(m_Device, m_UniformMemory[currentImage].data(), 0, sizeof(ubo), 0, &data);
   memcpy(data, &ubo, sizeof(ubo));
   vkUnmapMemory(m_Device, m_UniformMemory[currentImage].data());
}


void Renderer::DrawFrame() {
   vkWaitForFences(m_Device, 1, m_Fences[m_FrameIndex].ptr(), VK_TRUE, UINT64_MAX);

   uint32_t imageIndex;
   VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain.data(), UINT64_MAX,
                                           m_AcquireSemaphores[m_FrameIndex].data(), nullptr,
                                           &imageIndex);

   if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      //RecreateSwapchain(0, 0);
      return;
   }
   else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
   }

   updateUniformBuffer(imageIndex);

   VkSubmitInfo submitInfo = {};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

   recordCommandBuffer(m_GraphicsCmdBuffers[imageIndex], imageIndex);

   VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores = m_AcquireSemaphores[m_FrameIndex].ptr();
   submitInfo.pWaitDstStageMask = &waitStages;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &m_GraphicsCmdBuffers[imageIndex];
   submitInfo.signalSemaphoreCount = 1;
   submitInfo.pSignalSemaphores = m_ReleaseSemaphores[m_FrameIndex].ptr();


   vkResetFences(m_Device, 1, m_Fences[m_FrameIndex].ptr());

   if (vkQueueSubmit(m_Device.queue(QueueFamily::GRAPHICS), 1, &submitInfo, m_Fences[m_FrameIndex].data()) !=
       VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
   }

   VkPresentInfoKHR presentInfo = {};
   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

   presentInfo.waitSemaphoreCount = 1;
   presentInfo.pWaitSemaphores = m_ReleaseSemaphores[m_FrameIndex].ptr();
   presentInfo.swapchainCount = 1;
   presentInfo.pSwapchains = m_Swapchain.ptr();
   presentInfo.pImageIndices = &imageIndex;

   result = vkQueuePresentKHR(m_Device.queue(QueueFamily::PRESENT), &presentInfo);

   if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
      //RecreateSwapchain(0, 0);
   }
   else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
   }

   m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}