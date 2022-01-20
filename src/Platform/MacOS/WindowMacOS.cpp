#if defined (__APPLE__)

#include "Platform/MacOS/WindowMacOS.h"

#include <GLFW/glfw3.h>
#include <Engine/Renderer/utils.h>
#include <Engine/Events/WindowEvents.h>
#include <Engine/Events/KeyEvents.h>
#include <Engine/Events/MouseEvents.h>


AppWindow::AppWindow(uint32_t width, uint32_t height, const char* title) {
   InitializeGLFW({{GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE}});

   m_Window = glfwCreateWindow(static_cast<int>(width),
                               static_cast<int>(height), title, nullptr, nullptr);

   m_Context = GfxContext::Create(m_Window);
   m_Context->Init();

   glfwSetWindowUserPointer(m_Window, this);

   glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int newWidth, int newHeight) {
      auto *windowMac = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowMac->m_Width = newWidth;
      windowMac->m_Height = newHeight;
      windowMac->m_EventCallback(std::make_unique<WindowResizeEvent>(newWidth, newHeight));
   });

   glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
      auto *windowMac = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowMac->m_EventCallback(std::make_unique<WindowCloseEvent>());
   });

   glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int, int action, int) {
      auto *windowMac = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      switch (action) {
         case GLFW_PRESS:
            windowMac->m_EventCallback(std::make_unique<KeyPressEvent>(key, 0));
            break;

         case GLFW_RELEASE:
            windowMac->m_EventCallback(std::make_unique<KeyReleaseEvent>(key));
            break;

         case GLFW_REPEAT:
            windowMac->m_EventCallback(std::make_unique<KeyPressEvent>(key, 1));
            break;

         default:
            break;
      }
   });

   glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int) {
      auto *windowMac = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      switch (action) {
         case GLFW_PRESS:
            windowMac->m_EventCallback(std::make_unique<MouseButtonPressEvent>(button));
            break;

         case GLFW_RELEASE:
            windowMac->m_EventCallback(std::make_unique<MouseButtonReleaseEvent>(button));
            break;

         default:
            break;
      }
   });

   glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double offsetX, double offsetY) {
      auto *windowMac = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowMac->m_EventCallback(std::make_unique<MouseScrollEvent>(static_cast<float>(offsetX), static_cast<float>(offsetY)));
   });

   glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y) {
      auto *windowMac = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowMac->m_EventCallback(std::make_unique<MouseMoveEvent>(static_cast<float>(x), static_cast<float>(y)));
   });

   glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode) {
      auto *windowMac = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowMac->m_EventCallback(std::make_unique<CharacterPressEvent>(keycode));
   });
}


AppWindow::~AppWindow() { glfwDestroyWindow(m_Window); }

void AppWindow::OnUpdate() { glfwWaitEvents(); }

#endif