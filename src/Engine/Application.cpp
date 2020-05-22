#include "Application.h"
#include "Core.h"
#include "Renderer/utils.h"
#include <chrono>
#include <memory>
#include <thread>

Application *Application::s_Application;

Application::Application(const char *name) : m_ApplicationName(name) {
    if (s_Application) {
        std::ostringstream ss;
        ss << "[Engine] Application '" << s_Application->m_ApplicationName << " ["
           << s_Application << "]' already exists!";
        throw std::runtime_error(ss.str());
    }

    s_Application = this;
    Init();
}

Application::~Application() {
    std::cout << "[Engine] Terminating application..." << std::endl;
}

void Application::Init() {
    m_Window = Window::Create(800, 600, m_ApplicationName.data());
    m_Window->SetEventCallback([this](std::unique_ptr<Event> event) { OnEvent(std::move(event)); });
    m_Renderer = std::make_unique<Renderer>(static_cast<GfxContextVk &>(m_Window->Context()));

    std::cout << currentTime() << "[Engine] Initialized application" << std::endl;
}

void Application::Run() {
    m_Running = true;

    std::thread renderThread([this]() {
        std::chrono::milliseconds frameTiming(16);

        std::vector<VkCommandBuffer> submitBuffers;

        try {
            while (m_Running) {
                auto drawStart = TIME_NOW;

                ProcessEventQueue();

                submitBuffers.clear();

                auto imageIndex = m_Renderer->AcquireNextImage();
                m_LayerStack.UpdateLayers(imageIndex);
                m_LayerStack.DrawLayers(imageIndex, submitBuffers);
                m_Renderer->DrawFrame(submitBuffers);

                auto drawEnd = TIME_NOW;
                auto drawTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                        drawEnd - drawStart);
//                if (drawTime < frameTiming)
//                    std::this_thread::sleep_for(frameTiming - drawTime);
            }
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    });
    while (m_Running) {
        m_Window->OnUpdate();
    }
    renderThread.join();
}

void Application::OnEvent(std::unique_ptr<Event> e) {
    std::unique_lock<std::mutex> lock(m_WindowEventMutex);
    if (m_WindowEventQueue.size() < 50)
        m_WindowEventQueue.push(std::move(e));
}

auto Application::OnWindowClose(WindowCloseEvent &) -> bool {
    std::cout << "[Application] Closing window..." << std::endl;
    m_Running = false;
    return true;
}

auto Application::OnWindowResize(WindowResizeEvent &e) -> bool {
    std::cout << "[Application] Resizing window [" << e.Width() << ", " << e.Height() << "]" << std::endl;
    m_Renderer->OnWindowResize(e);
    return true;
}

void Application::ProcessEventQueue() {
    std::unique_lock<std::mutex> lock(m_WindowEventMutex);
    std::unique_ptr<Event> resizeEvent = nullptr;
    while (!m_WindowEventQueue.empty()) {
        auto event = std::move(m_WindowEventQueue.front());
        m_WindowEventQueue.pop();

        switch (event->Type()) {
            case EventType::None:
            case EventType::WindowMove:
                return;

            case EventType::WindowResize:
                resizeEvent = std::move(event);
                continue;
            case EventType::WindowClose:
                OnWindowClose(static_cast<WindowCloseEvent &>(*event));
                continue;

            case EventType::MouseButtonPress:
                OnMouseButtonPress(static_cast<MouseButtonPressEvent &>(*event));
                m_LayerStack.PropagateEvent(static_cast<MouseButtonPressEvent &>(*event),
                                            &Layer::OnMouseButtonPress);
                break;

            case EventType::MouseButtonRelease:
                OnMouseButtonRelease(static_cast<MouseButtonReleaseEvent &>(*event));
                m_LayerStack.PropagateEvent(
                        static_cast<MouseButtonReleaseEvent &>(*event),
                        &Layer::OnMouseButtonRelease);
                break;

            case EventType::MouseMove:
                OnMouseMove(static_cast<MouseMoveEvent &>(*event));
                m_LayerStack.PropagateEvent(static_cast<MouseMoveEvent &>(*event),
                                            &Layer::OnMouseMove);
                break;

            case EventType::MouseScroll:
                OnMouseScroll(static_cast<MouseScrollEvent &>(*event));
                m_LayerStack.PropagateEvent(static_cast<MouseScrollEvent &>(*event),
                                            &Layer::OnMouseScroll);
                break;

            case EventType::KeyPress:
                OnKeyPress(static_cast<KeyPressEvent &>(*event));
                m_LayerStack.PropagateEvent(static_cast<KeyPressEvent &>(*event),
                                            &Layer::OnKeyPress);
                break;

            case EventType::KeyRelease:
                OnKeyRelease(static_cast<KeyReleaseEvent &>(*event));
                m_LayerStack.PropagateEvent(static_cast<KeyReleaseEvent &>(*event),
                                            &Layer::OnKeyRelease);
                break;

            case EventType::CharacterPress:
                OnCharacterPress(static_cast<CharacterPressEvent &>(*event));
                m_LayerStack.PropagateEvent(static_cast<CharacterPressEvent &>(*event),
                                            &Layer::OnCharacterPress);
        }
    }

    if (resizeEvent) {
        OnWindowResize(static_cast<WindowResizeEvent &>(*resizeEvent));
        m_LayerStack.PropagateEvent(static_cast<WindowResizeEvent &>(*resizeEvent), &Layer::OnWindowResize);
    }
}
