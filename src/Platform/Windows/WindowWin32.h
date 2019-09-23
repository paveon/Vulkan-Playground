#ifndef VULKAN_WINDOWWIN32_H
#define VULKAN_WINDOWWIN32_H
#define GLFW_INCLUDE_VULKAN

#include <Engine/Window.h>
#include <Engine/Renderer/GraphicsContext.h>

struct GLFWwindow;

class WindowWin32 : public Window {
private:
    GLFWwindow* m_Window;
    EventCallbackFn m_EventCallback;

public:
    ~WindowWin32() override;

    WindowWin32(uint32_t width, uint32_t height, const char* title);

    void SetEventCallback(const EventCallbackFn& callback) override { m_EventCallback = callback; }

    void OnUpdate() override;

    void SetVSync() override {};

    void VSync() const override {};

    void* GetNativeHandle() const override { return m_Window; }
};


#endif //VULKAN_WINDOWWIN32_H
