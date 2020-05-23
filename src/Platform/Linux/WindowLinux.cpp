#include "WindowLinux.h"

#include <GLFW/glfw3.h>
#include <Engine/Renderer/utils.h>
#include <Engine/Events/WindowEvents.h>
#include <Engine/Events/KeyEvents.h>
#include <Engine/Events/MouseEvents.h>
#include <Platform/Vulkan/GraphicsContextVk.h>


WindowLinux::WindowLinux(uint32_t width, uint32_t height, const char* title) {
   InitializeGLFW();

   m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);

   m_Context = GfxContext::Create(m_Window);
   m_Context->Init();

   glfwSetWindowUserPointer(m_Window, this);
//   glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//   glfwSetFramebufferSizeCallback(m_Window, WindowLinux::ResizeCallback);

   glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int newWidth, int newHeight) {
       auto *windowLinux = static_cast<WindowLinux*>(glfwGetWindowUserPointer(window));
       windowLinux->m_Width = newWidth;
       windowLinux->m_Height = newHeight;
       windowLinux->m_EventCallback(std::make_unique<WindowResizeEvent>(newWidth, newHeight));
   });

   glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
       auto *windowLinux = static_cast<WindowLinux*>(glfwGetWindowUserPointer(window));
       windowLinux->m_EventCallback(std::make_unique<WindowCloseEvent>());
   });

   glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int, int action, int) {
       auto *windowLinux = static_cast<WindowLinux*>(glfwGetWindowUserPointer(window));
       switch (action) {
          case GLFW_PRESS:
             windowLinux->m_EventCallback(std::make_unique<KeyPressEvent>(key, 0));
             break;

          case GLFW_RELEASE:
             windowLinux->m_EventCallback(std::make_unique<KeyReleaseEvent>(key));
             break;

          case GLFW_REPEAT:
             windowLinux->m_EventCallback(std::make_unique<KeyPressEvent>(key, 1));
             break;

          default:
             break;
       }
   });

   glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int) {
       auto *windowLinux = static_cast<WindowLinux*>(glfwGetWindowUserPointer(window));
       switch (action) {
          case GLFW_PRESS:
             windowLinux->m_EventCallback(std::make_unique<MouseButtonPressEvent>(button));
             break;

          case GLFW_RELEASE:
             windowLinux->m_EventCallback(std::make_unique<MouseButtonReleaseEvent>(button));
             break;

          default:
             break;
       }
   });

   glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double offsetX, double offsetY) {
       auto *windowLinux = static_cast<WindowLinux*>(glfwGetWindowUserPointer(window));
       windowLinux->m_EventCallback(std::make_unique<MouseScrollEvent>(static_cast<float>(offsetX), static_cast<float>(offsetY)));
   });

   glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y) {
       auto *windowLinux = static_cast<WindowLinux*>(glfwGetWindowUserPointer(window));
       windowLinux->m_EventCallback(std::make_unique<MouseMoveEvent>(static_cast<float>(x), static_cast<float>(y)));
   });

   glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode) {
       auto *windowLinux = static_cast<WindowLinux*>(glfwGetWindowUserPointer(window));
       windowLinux->m_EventCallback(std::make_unique<CharacterPressEvent>(keycode));
   });
}


WindowLinux::~WindowLinux() {
   glfwDestroyWindow(m_Window);
}

void WindowLinux::OnUpdate() {
   glfwWaitEvents();
}
