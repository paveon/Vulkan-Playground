#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

#include "WindowWin32.h"

#include <GLFW/glfw3.h>
#include <Engine/Renderer/utils.h>
#include <Engine/Events/WindowEvents.h>
#include <Engine/Events/KeyEvents.h>
#include <Engine/Events/MouseEvents.h>


AppWindow::AppWindow(uint32_t width, uint32_t height, const char* title) {
   InitializeGLFW({});

   m_Window = glfwCreateWindow(static_cast<int>(width),
                               static_cast<int>(height), title, nullptr, nullptr);

   m_Context = GfxContext::Create(m_Window);
   m_Context->Init();

   glfwSetWindowUserPointer(m_Window, this);

   glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int newWidth, int newHeight) {
      auto* windowWin32 = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowWin32->m_Width = newWidth;
      windowWin32->m_Height = newHeight;
      windowWin32->m_EventCallback(std::make_unique<WindowResizeEvent>(newWidth, newHeight));
   });

   glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
      auto* windowWin32 = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowWin32->m_EventCallback(std::make_unique<WindowCloseEvent>());
   });

   glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int, int action, int) {
      auto* windowWin32 = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
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
      auto* windowWin32 = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
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
      auto* windowWin32 = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowWin32->m_EventCallback(
              std::make_unique<MouseScrollEvent>(static_cast<float>(offsetX), static_cast<float>(offsetY)));
   });

   glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y) {
      auto* windowWin32 = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowWin32->m_EventCallback(std::make_unique<MouseMoveEvent>(static_cast<float>(x), static_cast<float>(y)));
   });

   glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode) {
      auto* windowWin32 = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
      windowWin32->m_EventCallback(std::make_unique<CharacterPressEvent>(keycode));
   });
}


AppWindow::~AppWindow() { glfwDestroyWindow(m_Window); }

void AppWindow::OnUpdate() { glfwWaitEvents(); }

#endif