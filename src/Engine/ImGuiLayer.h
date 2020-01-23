#ifndef VULKAN_IMGUILAYER_H
#define VULKAN_IMGUILAYER_H

#include <imgui.h>
#include <mathlib.h>
#include <Engine/Renderer/Pipeline.h>

#include "Renderer/vulkan_wrappers.h"
#include "Input.h"
#include "Layer.h"
#include "KeyCodes.h"
#include "MouseButtonCodes.h"

class Renderer;

class RenderPass;

// Options and values to display/toggle from the UI
struct UISettings {
   bool displayModels = true;
   bool displayLogos = true;
   bool displayBackground = true;
   bool animateLight = false;
   float lightSpeed = 0.25f;
   std::array<float, 50> frameTimes{};
   float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
   float lightTimer = 0.0f;
};


class ImGuiLayer : public Layer {
private:
   Renderer& m_Renderer;
//    vk::Buffer vertexBuffer;
//    vk::Buffer indexBuffer;
//    vk::DeviceMemory vertexMemory;
//    vk::DeviceMemory indexMemory;
//    int32_t vertexCount = 0;
//    int32_t indexCount = 0;
   std::unique_ptr<Texture2D> m_FontData;
   std::unique_ptr<Pipeline> m_Pipeline;

public:
   struct PushConstBlock {
      math::vec2 scale;
      math::vec2 translate;
   } pushConstBlock;

   explicit ImGuiLayer(Renderer& renderer);

   ~ImGuiLayer() override;

   // Initialize styles, keys, etc.
   static void init(float width, float height);

   // Initialize all Vulkan resources used by the ui
   void initResources(const RenderPass& renderPass);

   // Starts a new imGui frame and sets up windows and ui elements
   void newFrame(float frameTimer, bool updateFrameGraph);

   // Update vertex and index buffer containing the imGui elements when required
   void updateBuffers();

   // Draw current imGui frame into a command buffer
   void drawFrame(VkCommandBuffer commandBuffer);

   void OnAttach() override;

   void OnUpdate() override {
      ImGuiIO& io = ImGui::GetIO();
      io.MouseDown[0] = Input::MouseButtonPressed(0);
      io.MouseDown[1] = Input::MouseButtonPressed(1);

      auto[x, y] = Input::MousePos();
      io.MousePos = ImVec2(x, y);

      //TODO
      //io.DeltaTime = frameTimer;
   }


   bool OnWindowResize(WindowResizeEvent& e) override {
      ImGuiIO& io = ImGui::GetIO();
      io.DisplaySize = ImVec2(e.Width(), e.Height());
      return true;
   }
};




// ----------------------------------------------------------------------------
// VulkanExample
// ----------------------------------------------------------------------------

//class VulkanExample {
//public:
//    uint32_t frameCounter = 0;
//
//    std::unique_ptr<ImGUI> imGui = nullptr;
//    bool prepared = false;
//
//    void buildCommandBuffers() {
//       VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
//
//       VkClearValue clearValues[2];
//       clearValues[0].color = {{0.2f, 0.2f, 0.2f, 1.0f}};
//       clearValues[1].depthStencil = {1.0f, 0};
//
//       VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
//       renderPassBeginInfo.renderPass = renderPass;
//       renderPassBeginInfo.renderArea.offset.x = 0;
//       renderPassBeginInfo.renderArea.offset.y = 0;
//       renderPassBeginInfo.renderArea.extent.width = 100;
//       renderPassBeginInfo.renderArea.extent.height = 100;
//       renderPassBeginInfo.clearValueCount = 2;
//       renderPassBeginInfo.pClearValues = clearValues;
//
//       imGui->newFrame(this, (frameCounter == 0));
//
//       imGui->updateBuffers();
//
//       for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
//          // Set target frame buffer
//          renderPassBeginInfo.framebuffer = frameBuffers[i];
//
//          VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
//
//          vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//          VkViewport viewport = vks::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
//          vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
//
//          VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
//          vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
//
//          // Render scene
//          vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
//          vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
//
//          // Render imGui
//          imGui->drawFrame(drawCmdBuffers[i]);
//
//          vkCmdEndRenderPass(drawCmdBuffers[i]);
//
//          VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
//       }
//    }
//
//    void draw() {
//       VulkanExampleBase::prepareFrame();
//       buildCommandBuffers();
//       submitInfo.commandBufferCount = 1;
//       submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
//       VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
//       VulkanExampleBase::submitFrame();
//    }
//
//    void prepareImGui() {
//       imGui = std::make_unique<ImGui>();
//       imGui->init((float) 100 (float) 100);
//       imGui->initResources(renderPass, queue);
//    }
//
//    void prepare() {
//       prepareImGui();
//       buildCommandBuffers();
//       prepared = true;
//    }
//
//    virtual void render() {
//       if (!prepared)
//          return;
//
//       // Update imGui
//       ImGuiIO& io = ImGui::GetIO();
//
//       io.DisplaySize = ImVec2((float) width, (float) height);
//       io.DeltaTime = frameTimer;
//
//       io.MousePos = ImVec2(mousePos.x, mousePos.y);
//       io.MouseDown[0] = mouseButtons.left;
//       io.MouseDown[1] = mouseButtons.right;
//
//       draw();
//    }
//
//    virtual void mouseMoved(double x, double y, bool& handled) {
//       ImGuiIO& io = ImGui::GetIO();
//       handled = io.WantCaptureMouse;
//    }
//
//};


#endif //VULKAN_IMGUILAYER_H