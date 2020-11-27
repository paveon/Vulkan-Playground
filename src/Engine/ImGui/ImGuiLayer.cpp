#include "ImGuiLayer.h"
#include <Platform/Vulkan/ImGuiLayerVk.h>

#include <memory>
#include <Engine/Renderer/RendererAPI.h>


auto ImGuiLayer::Create() -> std::unique_ptr<ImGuiLayer> {
    switch (RendererAPI::GetSelectedAPI()) {
        case RendererAPI::API::VULKAN:
            return std::make_unique<ImGuiLayerVk>();
    }

    return nullptr;
}


// Initialize styles, keys, etc.
void ImGuiLayer::ConfigureImGui(const std::pair<uint32_t, uint32_t> &framebufferSize) {
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(framebufferSize.first, framebufferSize.second);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.ConfigDockingWithShift = true;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
//    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();

    // Color scheme
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

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

void ImGuiLayer::OnAttach(const LayerStack* stack) {
    Layer::OnAttach(stack);
}