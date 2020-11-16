#ifndef VULKAN_IMGUILAYER_H
#define VULKAN_IMGUILAYER_H

#include <imgui.h>
#include <mathlib.h>
#include <vulkan/vulkan_core.h>
#include <Engine/Renderer/Renderer.h>


#include "Engine/Input.h"
#include "Engine/Layer.h"
#include "Engine/KeyCodes.h"
#include "Engine/MouseButtonCodes.h"


class RendererVk;

class RenderPass;


class ImGuiLayer : public Layer {
//    using DrawCallbackFunc = void (*)();

protected:
//    DrawCallbackFunc m_DrawCallback = []() {};

    explicit ImGuiLayer() : Layer("ImGuiLayer") {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
    }

    // Initialize styles, keys, etc.
    static void ConfigureImGui(const std::pair<uint32_t, uint32_t> &framebufferSize);

public:
    ~ImGuiLayer() override { ImGui::DestroyContext(); }

    static auto Create() -> std::unique_ptr<ImGuiLayer>;

//    void SetDrawCallback(DrawCallbackFunc cbFunc) { m_DrawCallback = cbFunc; }

    void OnAttach(const LayerStack *stack) override;

    virtual void NewFrame() = 0;

    virtual void EndFrame() = 0;

    void OnDraw() override {};

    auto OnMouseScroll(MouseScrollEvent &e) -> bool override {
        ImGuiIO &io = ImGui::GetIO();
        io.MouseWheel = e.OffsetY() / 2.0f;
        io.MouseWheelH = e.OffsetX() / 2.0f;
        e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
        return true;
    }

    auto OnMouseMove(MouseMoveEvent &e) -> bool override {
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = ImVec2(e.X(), e.Y());
        e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
        return true;
    }

    auto OnMouseButtonPress(MouseButtonPressEvent &e) -> bool override {
        ImGuiIO &io = ImGui::GetIO();
        io.MouseDown[e.Button()] = true;
        e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
        return true;
    }

    auto OnMouseButtonRelease(MouseButtonReleaseEvent &e) -> bool override {
        ImGuiIO &io = ImGui::GetIO();
        io.MouseDown[e.Button()] = false;
        e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
        return true;
    }

    auto OnKeyPress(KeyPressEvent &e) -> bool override {
        ImGuiIO &io = ImGui::GetIO();
        int keycode = e.KeyCode();
        io.KeysDown[keycode] = true;

        io.KeyCtrl = io.KeysDown[IO_KEY_LEFT_CONTROL] || io.KeysDown[IO_KEY_RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[IO_KEY_LEFT_SHIFT] || io.KeysDown[IO_KEY_RIGHT_SHIFT];
        io.KeyAlt = io.KeysDown[IO_KEY_LEFT_ALT] || io.KeysDown[IO_KEY_RIGHT_ALT];
        io.KeySuper = io.KeysDown[IO_KEY_LEFT_SUPER] || io.KeysDown[IO_KEY_RIGHT_SUPER];

        e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
        return true;
    }

    auto OnKeyRelease(KeyReleaseEvent &e) -> bool override {
        ImGuiIO &io = ImGui::GetIO();
        io.KeysDown[e.KeyCode()] = false;
        e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
        return true;
    }

    auto OnCharacterPress(CharacterPressEvent &e) -> bool override {
//        ImGuiIO &io = ImGui::GetIO();
//        io.AddInputCharacter(e.KeyCode());

        e.Handled = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
        return true;
    }

    auto OnWindowResize(WindowResizeEvent &e) -> bool override {
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2(e.Width(), e.Height());
        return true;
    }
};


#endif //VULKAN_IMGUILAYER_H