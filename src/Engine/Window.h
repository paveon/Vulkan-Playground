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

    Window& operator=(const Window& other) = delete;

    virtual ~Window() = default;

    static std::unique_ptr<Window> Create(uint32_t width, uint32_t height, const char* title);

    virtual void OnUpdate() = 0;

    uint32_t Width() const { return m_Width; };

    uint32_t Height() const { return m_Height; };

    std::pair<uint32_t, uint32_t> Size() const { return std::make_pair(m_Width, m_Height); }

    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

    virtual void SetVSync() = 0;

    virtual void VSync() const = 0;

    virtual void* GetNativeHandle() const = 0;

    const GfxContext& Context() const { return *m_Context; }
};


#endif //VULKAN_WINDOW_H
