#include "GraphicsContextVk.h"

#include <GLFW/glfw3.h>
#include <Engine/Renderer/utils.h>

GfxContextVk::GfxContextVk(GLFWwindow* windowHandle) : m_WindowHandle(windowHandle),
                                                       m_Instance(vk::Instance(ArrayToVector(s_VulkanValidationLayers))),
                                                       m_Surface(vk::Surface(m_Instance.data(), m_WindowHandle)),
                                                       m_Device(m_Instance, m_Surface){
}


void GfxContextVk::Init() {

}
