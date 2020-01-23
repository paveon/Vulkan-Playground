#include "GraphicsContextVk.h"

#include <GLFW/glfw3.h>
#include <Engine/Renderer/utils.h>

GfxContextVk::GfxContextVk(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle),
                                                       m_Instance(vk::Instance(true)),
                                                       m_Surface(vk::Surface(m_Instance.data(), m_WindowHandle)),
                                                       m_Device(m_Instance, m_Surface),
                                                       m_Swapchain(m_Device.createSwapChain(
                                                               {m_Device.GfxQueueIdx(), m_Device.PresentQueueIdx()},
                                                               chooseSwapExtent(
                                                                       FramebufferSizeVk(),
                                                                       m_Device.getSurfaceCapabilities()))) {
}


void GfxContextVk::Init() {
}
