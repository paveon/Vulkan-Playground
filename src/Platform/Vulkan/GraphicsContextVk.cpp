#include "GraphicsContextVk.h"

#include <GLFW/glfw3.h>
#include <Engine/Renderer/utils.h>

VkExtent2D GetFramebufferExtent(GLFWwindow* windowHandle) {
    int size[2] = {};
    glfwGetFramebufferSize(windowHandle, &size[0], &size[1]);

    return VkExtent2D {
            static_cast<uint32_t>(size[0]),
            static_cast<uint32_t>(size[1])
    };
}

GfxContextVk::GfxContextVk(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle),
                                                       m_Instance(vk::Instance(true)),
                                                       m_Surface(vk::Surface(m_Instance.data(), m_WindowHandle)),
                                                       m_Device(m_Instance, m_Surface),
                                                       m_Swapchain(m_Device.createSwapChain(
                                                               {m_Device.GfxQueueIdx(), m_Device.PresentQueueIdx()},
                                                               chooseSwapExtent(
                                                                       GetFramebufferExtent(m_WindowHandle),
                                                                       m_Device.getSurfaceCapabilities()))) {
}


void GfxContextVk::Init() {
}

auto GfxContextVk::RecreateSwapchain() -> void {
    m_Swapchain->Release();
    std::set queueIndices{ m_Device.GfxQueueIdx(), m_Device.PresentQueueIdx() };
    auto caps = m_Device.getSurfaceCapabilities();
    VkExtent2D extent = chooseSwapExtent(GetFramebufferExtent(m_WindowHandle),caps);
    m_Swapchain = m_Device.createSwapChain(queueIndices, extent);
}
