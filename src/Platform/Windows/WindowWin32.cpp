#include "WindowWin32.h"

#include <GLFW/glfw3.h>
#include <Engine/Renderer/utils.h>
#include <Engine/Events/WindowEvents.h>
#include <Engine/Events/KeyEvents.h>
#include <Engine/Events/MouseEvents.h>
#include <Platform/Vulkan/GraphicsContextVk.h>


WindowWin32::WindowWin32(uint32_t width, uint32_t height, const char* title) {
   InitializeGLFW();

   m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);

   m_Context = GfxContext::Create(m_Window);
   m_Context->Init();

   glfwSetWindowUserPointer(m_Window, this);
   //glfwSetFramebufferSizeCallback(m_Window, WindowWin32::ResizeCallback);

   glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int newWidth, int newHeight) {
       auto windowWin32 = static_cast<WindowWin32*>(glfwGetWindowUserPointer(window));
       windowWin32->m_Width = newWidth;
       windowWin32->m_Height = newHeight;
       windowWin32->m_EventCallback(std::make_unique<WindowResizeEvent>(newWidth, newHeight));
   });

   glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
       auto windowWin32 = static_cast<WindowWin32*>(glfwGetWindowUserPointer(window));
       windowWin32->m_EventCallback(std::make_unique<WindowCloseEvent>());
   });

   glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int, int action, int) {
       auto windowWin32 = static_cast<WindowWin32*>(glfwGetWindowUserPointer(window));
       switch (action) {
          case GLFW_PRESS:
             windowWin32->m_EventCallback(std::make_unique<KeyPressEvent>(key, 0));
             break;

          case GLFW_RELEASE:
             windowWin32->m_EventCallback(std::make_unique<KeyReleaseEvent>(key));
             break;

          case GLFW_REPEAT:
             windowWin32->m_EventCallback(std::make_unique<KeyPressEvent>(key, 1));
             break;

          default:
             break;
       }
   });

   glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int) {
       auto windowWin32 = static_cast<WindowWin32*>(glfwGetWindowUserPointer(window));
       switch (action) {
          case GLFW_PRESS:
             windowWin32->m_EventCallback(std::make_unique<MouseButtonPressEvent>(button));
             break;

          case GLFW_RELEASE:
             windowWin32->m_EventCallback(std::make_unique<MouseButtonReleaseEvent>(button));
             break;

          default:
             break;
       }
   });

   glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double offsetX, double offsetY) {
       auto windowWin32 = static_cast<WindowWin32*>(glfwGetWindowUserPointer(window));
       windowWin32->m_EventCallback(std::make_unique<MouseScrollEvent>(static_cast<float>(offsetX), static_cast<float>(offsetY)));
   });

   glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y) {
       auto windowWin32 = static_cast<WindowWin32*>(glfwGetWindowUserPointer(window));
       windowWin32->m_EventCallback(std::make_unique<MouseMoveEvent>(static_cast<float>(x), static_cast<float>(y)));
   });

   glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode) {
       auto windowWin32 = static_cast<WindowWin32*>(glfwGetWindowUserPointer(window));
       windowWin32->m_EventCallback(std::make_unique<CharacterPressEvent>(keycode));
   });
}


WindowWin32::~WindowWin32() {
   glfwDestroyWindow(m_Window);
}

void WindowWin32::OnUpdate() {
   glfwWaitEvents();
}
