#include "ImGuiLayer.h"

#include <algorithm>
#include <Platform/Vulkan/PipelineVk.h>
#include "Engine/Renderer/renderer.h"
#include "Engine/Renderer/Pipeline.h"
#include "Engine/Renderer/ShaderProgram.h"
#include "Application.h"


ImGuiLayer::ImGuiLayer(Renderer& renderer) : Layer("ImGuiLayer"), m_Renderer(renderer) {
   ImGui::CreateContext();
   m_Renderer.m_ImGui = this;
};


ImGuiLayer::~ImGuiLayer() {
   ImGui::DestroyContext();
}


// Initialize styles, keys, etc.
void ImGuiLayer::ConfigureImGui(const std::pair<uint32_t, uint32_t>& framebufferSize) {
   // Color scheme
   ImGuiStyle& style = ImGui::GetStyle();
   style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
   style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
   style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
   style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
   style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

   ImGuiIO& io = ImGui::GetIO();
   io.DisplaySize = ImVec2(framebufferSize.first, framebufferSize.second);
   io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

   io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
   io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

   io.KeyMap[ImGuiKey_Tab] = IO_KEY_TAB;
   io.KeyMap[ImGuiKey_LeftArrow] = IO_KEY_LEFT;
   io.KeyMap[ImGuiKey_RightArrow] = IO_KEY_RIGHT;
   io.KeyMap[ImGuiKey_UpArrow] = IO_KEY_UP;
   io.KeyMap[ImGuiKey_DownArrow] = IO_KEY_DOWN;
   io.KeyMap[ImGuiKey_PageUp] = IO_KEY_PAGE_UP;
   io.KeyMap[ImGuiKey_PageDown] = IO_KEY_PAGE_DOWN;
   io.KeyMap[ImGuiKey_Home] = IO_KEY_HOME;
   io.KeyMap[ImGuiKey_End] = IO_KEY_END;
   io.KeyMap[ImGuiKey_Insert] = IO_KEY_INSERT;
   io.KeyMap[ImGuiKey_Delete] = IO_KEY_DELETE;
   io.KeyMap[ImGuiKey_Backspace] = IO_KEY_BACKSPACE;
   io.KeyMap[ImGuiKey_Space] = IO_KEY_SPACE;
   io.KeyMap[ImGuiKey_Enter] = IO_KEY_ENTER;
   io.KeyMap[ImGuiKey_Escape] = IO_KEY_ESCAPE;
   io.KeyMap[ImGuiKey_KeyPadEnter] = IO_KEY_KP_ENTER;
   io.KeyMap[ImGuiKey_A] = IO_KEY_A;
   io.KeyMap[ImGuiKey_C] = IO_KEY_C;
   io.KeyMap[ImGuiKey_V] = IO_KEY_V;
   io.KeyMap[ImGuiKey_X] = IO_KEY_X;
   io.KeyMap[ImGuiKey_Y] = IO_KEY_Y;
   io.KeyMap[ImGuiKey_Z] = IO_KEY_Z;
}


//Initialize all Vulkan resources used by the ui
void ImGuiLayer::InitResources(const RenderPass& renderPass) {
   auto& gfxContext = static_cast<GfxContextVk&>(Application::GetGraphicsContext());
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
           VertexAttribute(ShaderType::UInt)
   };

   DescriptorLayout descriptorLayout{
           DescriptorBinding(DescriptorType::Texture)
   };

   std::unique_ptr<ShaderProgram> fragShader(ShaderProgram::Create("shaders/ui.frag.spv"));
   std::unique_ptr<ShaderProgram> vertexShader(ShaderProgram::Create("shaders/ui.vert.spv"));

   m_Pipeline = Pipeline::Create(renderPass, *vertexShader, *fragShader, vertexLayout, descriptorLayout, {pushBlock}, false);
   m_Pipeline->BindTexture(*m_FontData, 0);

   auto imgCount = gfxContext.Swapchain().ImageCount();
   m_VertexBuffers.resize(imgCount);
   m_IndexBuffers.resize(imgCount);
   m_VertexMemories.resize(imgCount);
   m_IndexMemories.resize(imgCount);
}

void ImGuiLayer::OnAttach() {
   Layer::OnAttach();
   ConfigureImGui(m_Renderer.FramebufferSize());
   InitResources(m_Renderer.GetRenderPass());
   std::cout << m_DebugName << "::Attached" << std::endl;
}


void ImGuiLayer::OnUpdate() {
   NewFrame();
   UpdateBuffers(m_Renderer.GetCurrentImageIndex());
}


void ImGuiLayer::NewFrame() {
   ImGui::NewFrame();
   m_DrawCallback();
   ImGui::ShowDemoWindow();
   ImGui::Render();
}


// Update vertex and index buffer containing the imGui elements when required
void ImGuiLayer::UpdateBuffers(size_t frameIndex) {
   ImDrawData* imDrawData = ImGui::GetDrawData();

   // Note: Alignment is done inside buffer creation
   VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
   VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

   if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
      return;
   }


   // Update buffers only if vertex or index count has been changed compared to current buffer size
   const auto& device = m_Renderer.GetDevice();
   if (vertexBufferSize > m_VertexBuffers[frameIndex].Size()) {
      m_VertexMemories[frameIndex].UnmapMemory();
      m_VertexBuffers[frameIndex] = device.createBuffer({device.queueIndex(QueueFamily::GRAPHICS)}, vertexBufferSize * 2,
                                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
      m_VertexMemories[frameIndex] = device.allocateBufferMemory(m_VertexBuffers[frameIndex], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      m_VertexBuffers[frameIndex].BindMemory(m_VertexMemories[frameIndex].data(), 0);
      m_VertexMemories[frameIndex].MapMemory(0, vertexBufferSize * 2);
   }

   if (indexBufferSize > m_IndexBuffers[frameIndex].Size()) {
      m_IndexMemories[frameIndex].UnmapMemory();
      m_IndexBuffers[frameIndex] = device.createBuffer({device.queueIndex(QueueFamily::GRAPHICS)}, indexBufferSize * 2,
                                                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
      m_IndexMemories[frameIndex] = device.allocateBufferMemory(m_IndexBuffers[frameIndex], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
      m_IndexBuffers[frameIndex].BindMemory(m_IndexMemories[frameIndex].data(), 0);
      m_IndexMemories[frameIndex].MapMemory(0, indexBufferSize);
   }

   // Upload data
   auto vtxDst = (ImDrawVert*) m_VertexMemories[frameIndex].m_Mapping;
   auto idxDst = (ImDrawIdx*) m_IndexMemories[frameIndex].m_Mapping;
   for (int n = 0; n < imDrawData->CmdListsCount; n++) {
      const ImDrawList* cmd_list = imDrawData->CmdLists[n];
      memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
      memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
      vtxDst += cmd_list->VtxBuffer.Size;
      idxDst += cmd_list->IdxBuffer.Size;
   }

   // Flush to make writes visible to GPU
   m_VertexMemories[frameIndex].Flush(0, VK_WHOLE_SIZE);
   m_IndexMemories[frameIndex].Flush(0, VK_WHOLE_SIZE);
}

// Draw current imGui frame into a command buffer
void ImGuiLayer::DrawFrame(VkCommandBuffer commandBuffer, size_t index) {
   ImGuiIO& io = ImGui::GetIO();

   m_Pipeline->Bind(commandBuffer, index);

   VkViewport viewport = {};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(ImGui::GetIO().DisplaySize.x);
   viewport.height = static_cast<float>(ImGui::GetIO().DisplaySize.y);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

   // UI scale and translate via push constants
   pushConstBlock.scale = math::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
   pushConstBlock.translate = math::vec2(-1.0f);
   m_Pipeline->PushConstants(commandBuffer, 0, sizeof(PushConstBlock), &pushConstBlock);

   // Render commands
   ImDrawData* imDrawData = ImGui::GetDrawData();
   int32_t vertexOffset = 0;
   int32_t indexOffset = 0;

   if (imDrawData->CmdListsCount > 0) {

      VkDeviceSize offsets[1] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, m_VertexBuffers[index].ptr(), offsets);
      vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffers[index].data(), 0, VK_INDEX_TYPE_UINT16);

      for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
         const ImDrawList* cmd_list = imDrawData->CmdLists[i];
         for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
            VkRect2D scissorRect;
            scissorRect.offset.x = std::max((int32_t) (pcmd->ClipRect.x), 0);
            scissorRect.offset.y = std::max((int32_t) (pcmd->ClipRect.y), 0);
            scissorRect.extent.width = (uint32_t) (pcmd->ClipRect.z - pcmd->ClipRect.x);
            scissorRect.extent.height = (uint32_t) (pcmd->ClipRect.w - pcmd->ClipRect.y);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
            vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
            indexOffset += pcmd->ElemCount;
         }
         vertexOffset += cmd_list->VtxBuffer.Size;
      }
   }
}