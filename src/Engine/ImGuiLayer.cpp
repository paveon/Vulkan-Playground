#include <algorithm>
#include "ImGuiLayer.h"
#include "Renderer/renderer.h"

using namespace vk;

static UISettings uiSettings;

ImGuiLayer::ImGuiLayer(Renderer& renderer) : m_Renderer(renderer) {
   ImGui::CreateContext();
};


ImGuiLayer::~ImGuiLayer() {
   ImGui::DestroyContext();
}


// Initialize styles, keys, etc.
void ImGuiLayer::init(float width, float height) {
   // Color scheme
   ImGuiStyle& style = ImGui::GetStyle();
   style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
   style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
   style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
   style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
   style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
   // Dimensions

   ImGuiIO& io = ImGui::GetIO();
   io.DisplaySize = ImVec2(width, height);
   io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}


// Initialize all Vulkan resources used by the ui
//void ImGuiLayer::initResources(VkRenderPass renderPass, VkQueue copyQueue) {
//   ImGuiIO& io = ImGui::GetIO();
//
//   // Create font texture
//   unsigned char* fontData;
//   int texWidth, texHeight;
//   io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
//   VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);
//
//   // Create target image for copy
//   VkImageCreateInfo imageInfo = {};
//   imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//   imageInfo.imageType = VK_IMAGE_TYPE_2D;
//   imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
//   imageInfo.extent.width = texWidth;
//   imageInfo.extent.height = texHeight;
//   imageInfo.extent.depth = 1;
//   imageInfo.mipLevels = 1;
//   imageInfo.arrayLayers = 1;
//   imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//   imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//   imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
//   imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//   imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//
//   fontImage = Image(m_Device, imageInfo);
//   fontMemory = m_Device.allocateImageMemory(fontImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//   vkBindImageMemory(m_Device, fontImage.data(), fontMemory.data(), 0);
//
//   fontView = fontImage.createView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
//
//   StagingBuffer stagingBuffer(m_Device, fontData, uploadSize);
//   CommandPool pool(m_Device, m_Device.queueIndex(QueueFamily::GRAPHICS));
//   CommandBuffer setupCmdBuffer(m_Device, pool.data());
//   setupCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
//
//   fontImage.ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT);
//   copyBufferToImage(setupCmdBuffer, stagingBuffer, fontImage);
//
//   fontImage.ChangeLayout(setupCmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
//   setupCmdBuffer.End();
//   setupCmdBuffer.Submit(copyQueue);
//   vkQueueWaitIdle(copyQueue);
//   sampler = Sampler(m_Device, 0.0);
//
//   std::vector<VkDescriptorPoolSize> poolSizes(1);
//   poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//   poolSizes[0].descriptorCount = 1;
//   descriptorPool = m_Device.createDescriptorPool(poolSizes, 2);
//
//   VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
//   samplerLayoutBinding.binding = 0;
//   samplerLayoutBinding.descriptorCount = 1;
//   samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//   samplerLayoutBinding.pImmutableSamplers = nullptr;
//   samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//   descriptorSetLayout = m_Device.createDescriptorSetLayout({samplerLayoutBinding});
//   descriptorSets = DescriptorSets(m_Device, descriptorPool.data(), {descriptorSetLayout.data()});
//
//   VkDescriptorImageInfo fontDescriptor = {};
//   fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//   fontDescriptor.imageView = fontView.data();
//   fontDescriptor.sampler = sampler.data();
//
//   std::array<VkWriteDescriptorSet, 1> writeDescriptorSets = {};
//   writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//   writeDescriptorSets[0].dstSet = descriptorSets[0];
//   writeDescriptorSets[0].dstBinding = 0;
//   writeDescriptorSets[0].dstArrayElement = 0;
//   writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//   writeDescriptorSets[0].descriptorCount = 1;
//   writeDescriptorSets[0].pImageInfo = &fontDescriptor;
//   vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
//
//   pipelineCache = PipelineCache(m_Device, 0, nullptr);
//
//   VkPushConstantRange pushConstantRange = {};
//   pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//   pushConstantRange.size = sizeof(PushConstBlock);
//   pushConstantRange.offset = 0;
//   pipelineLayout = PipelineLayout(m_Device, {descriptorSetLayout.data()}, {pushConstantRange});
//
//   VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
//   inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//   inputAssembly.primitiveRestartEnable = VK_FALSE;
//
//   VkPipelineRasterizationStateCreateInfo rasterizer = {};
//   rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//   rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Fill polygons
//   rasterizer.lineWidth = 1.0f; //Single fragment line width
//   rasterizer.cullMode = VK_CULL_MODE_NONE; //Back-face culling
//   rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //Front-facing polygons have vertices in clockwise order
//
//   VkPipelineColorBlendAttachmentState blendAttachmentState{};
//   blendAttachmentState.blendEnable = VK_TRUE;
//   blendAttachmentState.colorWriteMask =
//           VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//   blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//   blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//   blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
//   blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//   blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//   blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
//   VkPipelineColorBlendStateCreateInfo colorBlendState = {};
//   colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//   colorBlendState.attachmentCount = 1;
//   colorBlendState.pAttachments = &blendAttachmentState;
//
//   VkPipelineDepthStencilStateCreateInfo depthStencil = {};
//   depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//   depthStencil.depthTestEnable = VK_FALSE;
//   depthStencil.depthWriteEnable = VK_FALSE;
//   depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
//   depthStencil.front = depthStencil.back;
//   depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
//
//   VkPipelineViewportStateCreateInfo viewportState = {};
//   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//   viewportState.viewportCount = 1;
//   viewportState.scissorCount = 1;
//
//   VkPipelineMultisampleStateCreateInfo multisampling = {};
//   multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//   multisampling.rasterizationSamples = m_Device.maxSamples();
//
//   std::array<VkDynamicState, 2> dynamicStateEnables = {
//           VK_DYNAMIC_STATE_VIEWPORT,
//           VK_DYNAMIC_STATE_SCISSOR
//   };
//   VkPipelineDynamicStateCreateInfo dynamicState = {};
//   dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//   dynamicState.dynamicStateCount = dynamicStateEnables.size();
//   dynamicState.pDynamicStates = dynamicStateEnables.data();
//
//   VkVertexInputBindingDescription vInputBindDescription = {};
//   vInputBindDescription.binding = 0;
//   vInputBindDescription.stride = sizeof(ImDrawVert);
//   vInputBindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
//   std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{
//           {0, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(ImDrawVert, pos)},   // Location 0: Position
//           {1, 0, VK_FORMAT_R32G32_SFLOAT,  offsetof(ImDrawVert, uv)},   // Location 1: UV
//           {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)},   // Location 2: Color
//   };
//
//   VkPipelineVertexInputStateCreateInfo vertexInputState = {};
//   vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//   vertexInputState.vertexBindingDescriptionCount = 1;
//   vertexInputState.pVertexBindingDescriptions = &vInputBindDescription;
//   vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
//   vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();
//
//   ShaderModule vertShader(m_Device, "shaders/ui.vert.spv");
//   ShaderModule fragShader(m_Device, "shaders/ui.frag.spv");
//   std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{
//           vertShader.createInfo(VK_SHADER_STAGE_VERTEX_BIT),
//           fragShader.createInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
//   };
//
//   VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
//   pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//   pipelineCreateInfo.layout = pipelineLayout.data();
//   pipelineCreateInfo.renderPass = renderPass;
//   pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
//   pipelineCreateInfo.pRasterizationState = &rasterizer;
//   pipelineCreateInfo.pColorBlendState = &colorBlendState;
//   pipelineCreateInfo.pMultisampleState = &multisampling;
//   pipelineCreateInfo.pViewportState = &viewportState;
//   pipelineCreateInfo.pDepthStencilState = &depthStencil;
//   pipelineCreateInfo.pDynamicState = &dynamicState;
//   pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
//   pipelineCreateInfo.pStages = shaderStages.data();
//   pipelineCreateInfo.pVertexInputState = &vertexInputState;
//
//   pipeline = Pipeline(m_Device, pipelineCreateInfo, pipelineCache.data());
//}
//
//// Starts a new imGui frame and sets up windows and ui elements
//void ImGuiLayer::newFrame(float frameTimer, bool updateFrameGraph) {
//   ImGui::NewFrame();
//
//   // Init imGui windows and elements
//   ImGui::TextUnformatted("Example text");
//   ImGui::TextUnformatted(m_Device.Name());
//
//   // Update frame time display
//   if (updateFrameGraph) {
//      std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
//      float frameTime = 1000.0f / (frameTimer * 1000.0f);
//      uiSettings.frameTimes.back() = frameTime;
//      if (frameTime < uiSettings.frameTimeMin) {
//         uiSettings.frameTimeMin = frameTime;
//      }
//      if (frameTime > uiSettings.frameTimeMax) {
//         uiSettings.frameTimeMax = frameTime;
//      }
//   }
//
//   ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));
//
//   ImGui::Text("Camera");
//   float cameraPos[3] = {0.0f, 1.0f, 3.0f};
//   float cameraRot[3] = {0.3f, 0.0f, 0.8f};
//   ImGui::InputFloat3("position", cameraPos, 2);
//   ImGui::InputFloat3("rotation", cameraRot, 2);
//
//   ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
//   ImGui::Begin("Example settings");
//   ImGui::Checkbox("Render models", &uiSettings.displayModels);
//   ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
//   ImGui::Checkbox("Display background", &uiSettings.displayBackground);
//   ImGui::Checkbox("Animate light", &uiSettings.animateLight);
//   ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
//   ImGui::End();
//
//   ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
//   ImGui::ShowDemoWindow();
//
//   // Render to generate draw buffers
//   ImGui::Render();
//}
//
//// Update vertex and index buffer containing the imGui elements when required
//void ImGuiLayer::updateBuffers() {
//   static void* vertData = nullptr;
//   static void* idxData = nullptr;
//   ImDrawData* imDrawData = ImGui::GetDrawData();
//
//   // Note: Alignment is done inside buffer creation
//   VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
//   VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
//
//   if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
//      return;
//   }
//
//   // Update buffers only if vertex or index count has been changed compared to current buffer size
//   if (!vertexBuffer.data() || vertexCount != imDrawData->TotalVtxCount) {
//      if (vertexMemory.data()) vkUnmapMemory(m_Device, vertexMemory.data());
//      vertexBuffer = Buffer(m_Device, {m_Device.queueIndex(QueueFamily::GRAPHICS)}, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
//      vertexMemory = m_Device.allocateBufferMemory(vertexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
//      vkBindBufferMemory(m_Device, vertexBuffer.data(), vertexMemory.data(), 0);
//      vertexCount = imDrawData->TotalVtxCount;
//      vkMapMemory(m_Device, vertexMemory.data(), 0, vertexBufferSize, 0, &vertData);
//   }
//
//   if (!indexBuffer.data() || indexCount != imDrawData->TotalIdxCount) {
//      if (indexMemory.data()) vkUnmapMemory(m_Device, indexMemory.data());
//      indexBuffer = Buffer(m_Device, {m_Device.queueIndex(QueueFamily::GRAPHICS)}, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
//      indexMemory = m_Device.allocateBufferMemory(indexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
//      vkBindBufferMemory(m_Device, indexBuffer.data(), indexMemory.data(), 0);
//      indexCount = imDrawData->TotalIdxCount;
//      vkMapMemory(m_Device, indexMemory.data(), 0, indexBufferSize, 0, &idxData);
//   }
//
//   // Upload data
//   auto vtxDst = (ImDrawVert*) vertData;
//   auto idxDst = (ImDrawIdx*) idxData;
//
//   for (int n = 0; n < imDrawData->CmdListsCount; n++) {
//      const ImDrawList* cmd_list = imDrawData->CmdLists[n];
//      memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
//      memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
//      vtxDst += cmd_list->VtxBuffer.Size;
//      idxDst += cmd_list->IdxBuffer.Size;
//   }
//
//   // Flush to make writes visible to GPU
//   VkMappedMemoryRange mappedRange = {};
//   mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
//   mappedRange.memory = vertexMemory.data();
//   mappedRange.offset = 0;
//   mappedRange.size = VK_WHOLE_SIZE;
//   vkFlushMappedMemoryRanges(m_Device, 1, &mappedRange);
//   mappedRange.memory = indexMemory.data();
//   vkFlushMappedMemoryRanges(m_Device, 1, &mappedRange);
//}
//
//// Draw current imGui frame into a command buffer
//void ImGuiLayer::drawFrame(VkCommandBuffer commandBuffer) {
//   ImGuiIO& io = ImGui::GetIO();
//
//   vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout.data(),
//                           0, 1, descriptorSets.data(), 0, nullptr);
//   vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.data());
//
//   VkViewport viewport = {};
//   viewport.x = 0.0f;
//   viewport.y = 0.0f;
//   viewport.width = static_cast<float>(ImGui::GetIO().DisplaySize.x);
//   viewport.height = static_cast<float>(ImGui::GetIO().DisplaySize.y);
//   viewport.minDepth = 0.0f;
//   viewport.maxDepth = 1.0f;
//   vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
//
//   // UI scale and translate via push constants
//   pushConstBlock.scale = math::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
//   pushConstBlock.translate = math::vec2(-1.0f);
//   vkCmdPushConstants(commandBuffer, pipelineLayout.data(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);
//
//   // Render commands
//   ImDrawData* imDrawData = ImGui::GetDrawData();
//   int32_t vertexOffset = 0;
//   int32_t indexOffset = 0;
//
//   if (imDrawData->CmdListsCount > 0) {
//
//      VkDeviceSize offsets[1] = {0};
//      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.ptr(), offsets);
//      vkCmdBindIndexBuffer(commandBuffer, indexBuffer.data(), 0, VK_INDEX_TYPE_UINT16);
//
//      for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
//         const ImDrawList* cmd_list = imDrawData->CmdLists[i];
//         for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
//            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
//            VkRect2D scissorRect;
//            scissorRect.offset.x = std::max((int32_t) (pcmd->ClipRect.x), 0);
//            scissorRect.offset.y = std::max((int32_t) (pcmd->ClipRect.y), 0);
//            scissorRect.extent.width = (uint32_t) (pcmd->ClipRect.z - pcmd->ClipRect.x);
//            scissorRect.extent.height = (uint32_t) (pcmd->ClipRect.w - pcmd->ClipRect.y);
//            vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
//            vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
//            indexOffset += pcmd->ElemCount;
//         }
//         vertexOffset += cmd_list->VtxBuffer.Size;
//      }
//   }
//}