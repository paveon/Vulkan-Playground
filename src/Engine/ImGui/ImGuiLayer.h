#ifndef VULKAN_IMGUILAYER_H
#define VULKAN_IMGUILAYER_H

#include <imgui.h>
#include <mathlib.h>
#include <Engine/Renderer/Pipeline.h>

#include "Engine/Renderer/vulkan_wrappers.h"
#include "Engine/Input.h"
#include "Engine/Layer.h"
#include "Engine/KeyCodes.h"
#include "Engine/MouseButtonCodes.h"

class Renderer;

class RenderPass;

class ImGuiLayer : public Layer {
private:
    using DrawCallbackFunc = void (*)();

    Renderer& m_Renderer;
    std::vector<vk::Buffer> m_VertexBuffers;
    std::vector<vk::Buffer> m_IndexBuffers;
    std::vector<vk::DeviceMemory> m_VertexMemories;
    std::vector<vk::DeviceMemory> m_IndexMemories;
    vk::DescriptorPool* m_DescriptorPool;
    VkRenderPass m_RenderPass = nullptr;

    vk::CommandPool* m_CmdPool;
    vk::CommandBuffers* m_CmdBuffers;
    std::vector<vk::Framebuffer*> m_Framebuffers;

//    std::unique_ptr<Texture2D> m_FontData;
//    std::unique_ptr<Pipeline> m_Pipeline;

    DrawCallbackFunc m_DrawCallback = []() {};

    // Initialize styles, keys, etc.
    static void ConfigureImGui(const std::pair<uint32_t, uint32_t>& framebufferSize);

    // Initialize all Vulkan resources used by the ui
    void InitResources(const RenderPass& renderPass);

public:

    struct PushConstBlock {
        math::vec2 scale;
        math::vec2 translate;
    } pushConstBlock;

    explicit ImGuiLayer(Renderer& renderer);

    ~ImGuiLayer() override;

    void SetDrawCallback(DrawCallbackFunc cbFunc) { m_DrawCallback = cbFunc; }

    // Update vertex and index buffer containing the imGui elements when required
    void UpdateBuffers(size_t frameIndex);

    // Draw current ImGui frame into a command buffer
    auto RecordBuffer(size_t index) -> VkCommandBuffer;

    void OnAttach() override;

    auto OnDraw(uint32_t imageIndex) -> VkCommandBuffer override;

    auto OnMouseScroll(MouseScrollEvent& e) -> bool override {
       ImGuiIO& io = ImGui::GetIO();
       io.MouseWheel = e.OffsetY() / 2.0f;
       io.MouseWheelH = e.OffsetX() / 2.0f;
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    auto OnMouseMove(MouseMoveEvent& e) -> bool override {
       ImGuiIO& io = ImGui::GetIO();
       io.MousePos = ImVec2(e.X(), e.Y());
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    auto OnMouseButtonPress(MouseButtonPressEvent& e) -> bool override {
       ImGuiIO& io = ImGui::GetIO();
       io.MouseDown[e.Button()] = true;
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    auto OnMouseButtonRelease(MouseButtonReleaseEvent& e) -> bool override {
       ImGuiIO& io = ImGui::GetIO();
       io.MouseDown[e.Button()] = false;
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    auto OnKeyPress(KeyPressEvent& e) -> bool override {
       ImGuiIO& io = ImGui::GetIO();
       int keycode = e.KeyCode();
       io.KeysDown[keycode] = true;

       io.KeyCtrl = io.KeysDown[IO_KEY_LEFT_CONTROL] || io.KeysDown[IO_KEY_RIGHT_CONTROL];
       io.KeyShift = io.KeysDown[IO_KEY_LEFT_SHIFT] || io.KeysDown[IO_KEY_RIGHT_SHIFT];
       io.KeyAlt = io.KeysDown[IO_KEY_LEFT_ALT] || io.KeysDown[IO_KEY_RIGHT_ALT];
       io.KeySuper = io.KeysDown[IO_KEY_LEFT_SUPER] || io.KeysDown[IO_KEY_RIGHT_SUPER];

       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    auto OnKeyRelease(KeyReleaseEvent& e) -> bool override {
       ImGuiIO& io = ImGui::GetIO();
       io.KeysDown[e.KeyCode()] = false;
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    auto OnCharacterPress(CharacterPressEvent& e) -> bool override {
       ImGuiIO& io = ImGui::GetIO();
       io.AddInputCharacter(e.KeyCode());

       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    auto OnWindowResize(WindowResizeEvent& e) -> bool override {
       ImGuiIO& io = ImGui::GetIO();
       io.DisplaySize = ImVec2(e.Width(), e.Height());
       return true;
    }
};


#endif //VULKAN_IMGUILAYER_H