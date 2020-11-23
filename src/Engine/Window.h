#ifndef VULKAN_WINDOW_H
#define VULKAN_WINDOW_H

#include <string>
#include <cstdint>
#include <memory>
#include <functional>

#include "Renderer/GraphicsContext.h"

class Event;

class Window {
protected:
    uint32_t m_Width = 1280;
    uint32_t m_Height = 720;
    std::string m_Title = "Window";
    std::unique_ptr<GfxContext> m_Context;

    Window() = default;

public:
    using EventCallbackFn = std::function<void(std::unique_ptr<Event>)>;

    Window(const Window& other) = delete;

    auto operator=(const Window& other) -> Window& = delete;

    virtual ~Window() = default;

    static auto Create(uint32_t width, uint32_t height, const char* title) -> std::unique_ptr<Window>;

    virtual void OnUpdate() = 0;

    auto Width() const -> uint32_t { return m_Width; };

    auto Height() const -> uint32_t { return m_Height; };

    auto Size() const -> std::pair<uint32_t, uint32_t> { return std::make_pair(m_Width, m_Height); }

    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

    virtual void SetVSync() = 0;

    virtual void VSync() const = 0;

    virtual auto GetNativeHandle() const -> void* = 0;

    auto Context() -> GfxContext& { return *m_Context; }
};


#endif //VULKAN_WINDOW_H
