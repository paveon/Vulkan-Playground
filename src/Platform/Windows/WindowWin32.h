#ifndef VULKAN_WINDOWWIN32_H
#define VULKAN_WINDOWWIN32_H

#include <memory>

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <Engine/Window.h>


class WindowWin32 : public Window {
private:
    GLFWwindow* m_Window;
    EventCallbackFn m_EventCallback;

public:
    ~WindowWin32() override { glfwDestroyWindow(m_Window); }

    WindowWin32(uint32_t width, uint32_t height, const char* title);

    void SetEventCallback(const EventCallbackFn& callback) override { m_EventCallback = callback; }

    void OnUpdate() override { glfwWaitEvents(); }

    void SetVSync() override {};

    void VSync() const override {};

    VkExtent2D FramebufferSize() const override {
       int width, height;
       glfwGetFramebufferSize(m_Window, &width, &height);
       return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    Surface CreateVulkanSurface(VkInstance instance) const override { return Surface(instance, m_Window); }

    bool ShouldClose() const override { return glfwWindowShouldClose(m_Window); }
};


#endif //VULKAN_WINDOWWIN32_H
