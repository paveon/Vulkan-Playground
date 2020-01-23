#include "ImGuiLayer.h"

#include <algorithm>
#include "Engine/Renderer/renderer.h"
#include "Engine/Renderer/Pipeline.h"
#include "Engine/Renderer/ShaderProgram.h"
#include "Application.h"


static UISettings uiSettings;

ImGuiLayer::ImGuiLayer(Renderer& renderer) : Layer("ImGuiLayer"),  m_Renderer(renderer) {
   ImGui::CreateContext();
   auto size = Application::GetGraphicsContext().FramebufferSize();
   init(size.first, size.second);
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


//Initialize all Vulkan resources used by the ui
void ImGuiLayer::initResources(const RenderPass& renderPass) {
   ImGuiIO& io = ImGui::GetIO();

   // Create font texture
   unsigned char* fontData;
   int texWidth, texHeight;
   io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
   m_FontData = Texture2D::Create(fontData, texWidth, texHeight);
   m_FontData->Upload();

   PushConstant pushBlock = {sizeof(PushConstBlock)};

   VertexLayout vertexLayout{
           VertexAttribute(ShaderType::Float2),
           VertexAttribute(ShaderType::Float2),
           VertexAttribute(ShaderType::Float4)
   };

   DescriptorLayout descriptorLayout{
           DescriptorBinding(DescriptorType::Texture)
   };

   std::unique_ptr<ShaderProgram> fragShader(ShaderProgram::Create("shaders/ui.frag.spv"));
   std::unique_ptr<ShaderProgram> vertexShader(ShaderProgram::Create("shaders/ui.vert.spv"));

   m_Pipeline = Pipeline::Create(renderPass, *vertexShader, *fragShader, vertexLayout, descriptorLayout, {pushBlock});
   m_Pipeline->BindTexture(*m_FontData, 0);
}

void ImGuiLayer::OnAttach() {
   Layer::OnAttach();
   initResources(m_Renderer.GetRenderPass());
   std::cout << m_DebugName << "::Attached" << std::endl;
}

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