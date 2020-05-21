#ifndef VULKAN_WINDOW_LINUX_H
#define VULKAN_WINDOW_LINUX_H
#define GLFW_INCLUDE_VULKAN

#include <Engine/Window.h>
#include <Engine/Renderer/GraphicsContext.h>

struct GLFWwindow;

class WindowLinux : public Window {
private:
    GLFWwindow* m_Window;
    EventCallbackFn m_EventCallback;

public:
    ~WindowLinux() override;

    WindowLinux(uint32_t width, uint32_t height, const char* title);

    void SetEventCallback(const EventCallbackFn& callback) override { m_EventCallback = callback; }

    void OnUpdate() override;

    void SetVSync() override {};

    void VSync() const override {};

    auto GetNativeHandle() const -> void* override { return m_Window; }
};


#endif //VULKAN_WINDOW_LINUX_H
