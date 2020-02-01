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

class ImGuiLayer : public Layer {
private:
    using DrawCallbackFunc = void (*)();

    Renderer& m_Renderer;
    std::vector<vk::Buffer> m_VertexBuffers;
    std::vector<vk::Buffer> m_IndexBuffers;
    std::vector<vk::DeviceMemory> m_VertexMemories;
    std::vector<vk::DeviceMemory> m_IndexMemories;

    std::unique_ptr<Texture2D> m_FontData;
    std::unique_ptr<Pipeline> m_Pipeline;

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

    // Starts a new imGui frame and sets up windows and ui elements
    void NewFrame();

    // Update vertex and index buffer containing the imGui elements when required
    void UpdateBuffers(size_t frameIndex);

    // Draw current ImGui frame into a command buffer
    void DrawFrame(VkCommandBuffer commandBuffer, size_t index);

    void OnAttach() override;

    void OnUpdate() override;

    bool OnMouseScroll(MouseScrollEvent& e) override {
       ImGuiIO& io = ImGui::GetIO();
       io.MouseWheel = e.OffsetY() / 2.0f;
       io.MouseWheelH = e.OffsetX() / 2.0f;
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    bool OnMouseMove(MouseMoveEvent& e) override {
       ImGuiIO& io = ImGui::GetIO();
       io.MousePos = ImVec2(e.X(), e.Y());
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    bool OnMouseButtonPress(MouseButtonPressEvent& e) override {
       ImGuiIO& io = ImGui::GetIO();
       io.MouseDown[e.Button()] = true;
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    bool OnMouseButtonRelease(MouseButtonReleaseEvent& e) override {
       ImGuiIO& io = ImGui::GetIO();
       io.MouseDown[e.Button()] = false;
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    bool OnKeyPress(KeyPressEvent& e) override {
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

    bool OnKeyRelease(KeyReleaseEvent& e) override {
       ImGuiIO& io = ImGui::GetIO();
       io.KeysDown[e.KeyCode()] = false;
       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    bool OnCharacterPress(CharacterPressEvent& e) override {
       ImGuiIO& io = ImGui::GetIO();
       io.AddInputCharacter(e.KeyCode());

       e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
       return true;
    }

    bool OnWindowResize(WindowResizeEvent& e) override {
       ImGuiIO& io = ImGui::GetIO();
       io.DisplaySize = ImVec2(e.Width(), e.Height());
       return true;
    }
};


#endif //VULKAN_IMGUILAYER_H