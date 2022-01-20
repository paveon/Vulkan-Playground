#ifndef VULKAN_WINDOWWIN32_H
#define VULKAN_WINDOWWIN32_H

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

#define GLFW_INCLUDE_VULKAN

#include <string>
#include <Engine/Renderer/GraphicsContext.h>


struct GLFWwindow;

class Event;

class AppWindow {
    using EventCallbackFn = std::function<void(std::unique_ptr<Event>)>;

    uint32_t m_Width = 1280;
    uint32_t m_Height = 720;
    std::string m_Title = "Window";
    std::unique_ptr<GfxContext> m_Context;

    GLFWwindow* m_Window{};
    EventCallbackFn m_EventCallback;

public:
    AppWindow(uint32_t width, uint32_t height, const char* title);

    AppWindow(const AppWindow& other) = delete;

    auto operator=(const AppWindow& other) -> AppWindow& = delete;

    ~AppWindow();

    void OnUpdate();

    auto Width() const -> uint32_t { return m_Width; };

    auto Height() const -> uint32_t { return m_Height; };

    auto Size() const -> std::pair<uint32_t, uint32_t> { return std::make_pair(m_Width, m_Height); }

    void SetEventCallback(const EventCallbackFn& callback) { m_EventCallback = callback; }

    void SetVSync() {}

    void VSync() const {}

    auto GetNativeHandle() const -> void* { return m_Window; }

    auto Context() -> GfxContext& { return *m_Context; }
};

#endif
#endif //VULKAN_WINDOWWIN32_H
