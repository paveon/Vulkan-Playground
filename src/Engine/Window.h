#ifndef VULKAN_WINDOW_H
#define VULKAN_WINDOW_H

#include <string>
#include <cstdint>
#include <memory>
#include <Renderer/vulkan_wrappers.h>

class Event;

class Window {
protected:
    uint32_t m_Width = 1280;
    uint32_t m_Height = 720;
    std::string m_Title = "Window";

    Window() = default;

public:
    using EventCallbackFn = std::function<void(std::unique_ptr<Event>)>;

    struct Extent2D {
        uint32_t width;
        uint32_t height;
    };

    Window(const Window& other) = delete;
    Window& operator=(const Window& other) = delete;

    virtual ~Window() = default;
    virtual void OnUpdate() = 0;
    uint32_t Width() const { return m_Width; };
    uint32_t Height() const { return m_Height; };

    virtual VkExtent2D FramebufferSize() const = 0;

    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
    virtual void SetVSync() = 0;
    virtual void VSync() const = 0;
    virtual Surface CreateVulkanSurface(VkInstance instance) const = 0;

    static std::unique_ptr<Window> Create(uint32_t width, uint32_t height, const char* title);

    virtual bool ShouldClose() const = 0;
};


#endif //VULKAN_WINDOW_H
