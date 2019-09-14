#include <chrono>
#include <thread>
#include <memory>
#include <Renderer/utils.h>
#include "Application.h"
#include "Core.h"


Application::Application() : m_Instance(ArrayToVector(s_VulkanValidationLayers)),
                             m_Window(Window::Create(WindowWidth, WindowHeight, "Renderer")),
                             m_Renderer(m_Instance.data(), m_Window, m_Window->FramebufferSize()) {
   std::cout << currentTime() << "[Engine] Initialized application" << std::endl;
   m_Window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));
}


Application::~Application() {
   std::cout << "[Engine] Terminating application..." << std::endl;
}


void Application::Run() {
   m_Running = true;

   std::thread renderThread([this](){
       std::chrono::milliseconds frameTiming(16);
       try {
          while (m_Running) {
             auto drawStart = TIME_NOW;

             ProcessEventQueue();
             m_Renderer.DrawFrame();

             auto drawEnd = TIME_NOW;
             auto drawTime = std::chrono::duration_cast<std::chrono::milliseconds>(drawEnd - drawStart);
             if (drawTime < frameTiming) std::this_thread::sleep_for(frameTiming - drawTime);

          }
       } catch (const std::exception& e) {
          std::cerr << e.what() << std::endl;
       }
   });
   while (m_Running) { m_Window->OnUpdate(); }
   renderThread.join();
}


void Application::OnEvent(std::unique_ptr<Event> e) {
   std::unique_lock<std::mutex> lock(m_WindowEventMutex);
   if (m_WindowEventQueue.size() < 50) m_WindowEventQueue.push(std::move(e));
}

bool Application::OnWindowClose(const WindowCloseEvent&) {
   std::cout << "[Application] Closing window..." << std::endl;
   m_Running = false;
   return true;
}

bool Application::OnWindowResize(const WindowResizeEvent& e) {
   m_Renderer.RecreateSwapchain(e.Width(), e.Height());
   return true;
}

void Application::ProcessEventQueue() {
   std::unique_lock<std::mutex> lock(m_WindowEventMutex);
   std::unique_ptr<Event> resizeEvent = nullptr;
   while (!m_WindowEventQueue.empty()) {
      auto event = std::move(m_WindowEventQueue.front());
      m_WindowEventQueue.pop();
      switch (event->Type()) {
         case EventType::WindowResize: resizeEvent = std::move(event); break;
         case EventType::WindowClose: OnWindowClose(static_cast<const WindowCloseEvent&>(*event)); break;
         case EventType::MouseButtonPress: OnMouseButtonPress(static_cast<const MouseButtonPressEvent&>(*event)); break;
         case EventType::MouseButtonRelease: OnMouseButtonRelease(static_cast<const MouseButtonReleaseEvent&>(*event)); break;
         case EventType::MouseMove: OnMouseMove(static_cast<const MouseMoveEvent&>(*event)); break;
         case EventType::MouseScroll: OnMouseScroll(static_cast<const MouseScrollEvent&>(*event)); break;
         case EventType::KeyPress: OnKeyPress(static_cast<const KeyPressEvent&>(*event)); break;
         case EventType::KeyRelease: OnKeyRelease(static_cast<const KeyReleaseEvent&>(*event)); break;

         default:
            break;
      }
   }
   if (resizeEvent) OnWindowResize(static_cast<const WindowResizeEvent&>(*resizeEvent));
}

