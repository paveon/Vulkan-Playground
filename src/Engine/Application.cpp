#include "Application.h"
#include "Core.h"
#include <chrono>
#include <memory>
#include <thread>
#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/utils.h>
#include <gtk/gtk.h>


Application *Application::s_Application;


Application::Application(const char *name) : m_Name(name), m_LayerStack(this) {
    if (s_Application) {
        std::ostringstream ss;
        ss << "[Application] '" << s_Application->m_Name << " [" << s_Application << "]' already exists!";
        throw std::runtime_error(ss.str());
    }

    s_Application = this;
    Init();
}

Application::~Application() {
    std::cout << currentTime() << "[" << s_Application->m_Name << "] Exiting" << std::endl;
    Renderer::Destroy();

    g_object_unref (m_GtkApp);
}

void Application::Init() {
    m_Window = AppWindow::Create(800, 600, m_Name.data());
    m_Window->SetEventCallback([this](std::unique_ptr<Event> event) { OnEvent(std::move(event)); });
//    m_Renderer = std::make_unique<RendererVk>(static_cast<GfxContextVk &>(m_Window->Context()));

    Renderer::Init();

    m_GtkApp = gtk_application_new("org.gtkmm.example", GApplicationFlags::G_APPLICATION_FLAGS_NONE);
    gtk_init(nullptr, nullptr);

    std::cout << currentTime() << "[" << s_Application->m_Name << "] Initialized" << std::endl;
}

void Application::Run() {
    m_Running = true;

    std::thread renderThread([this]() {
//        std::chrono::milliseconds frameTiming(16);

        std::vector<VkCommandBuffer> submitBuffers;

        try {
            while (m_Running) {
                auto currentTime = TIME_NOW;
                Timestep timestep(currentTime - m_LastFrameTime);
                m_LastFrameTime = currentTime;

                ProcessEventQueue();

                Renderer::NewFrame();
                m_LayerStack.UpdateLayers(timestep);
                m_LayerStack.DrawLayers();
                Renderer::PresentFrame();

//                submitBuffers.clear();

//                auto imageIndex = m_Renderer->AcquireNextImage();

                // Inheritance info for the secondary command buffers
//                VkCommandBufferInheritanceInfo inheritanceInfo = {};
//                inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
//                inheritanceInfo.renderPass = (VkRenderPass) m_Renderer->GetRenderPass().VkHandle();
//                inheritanceInfo.framebuffer = m_Renderer->GetFramebuffer().data();
//
//                m_LayerStack.UpdateLayers(timestep, imageIndex);
//                m_LayerStack.DrawLayers(imageIndex, submitBuffers, inheritanceInfo);
//                m_Renderer->DrawFrame(submitBuffers);

//                auto drawEnd = TIME_NOW;
//                auto drawTime = std::chrono::duration_cast<std::chrono::microseconds>(drawEnd - currentTime);
//                double frameTime = drawTime.count() / 1000000.0;
//                printf("Frametime: %f, FPS: %d\n", frameTime, (uint32_t)(1.0 / frameTime));
//                if (drawTime < frameTiming)
//                    std::this_thread::sleep_for(frameTiming - drawTime);
            }

            Renderer::WaitIdle();

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

void Application::OnWindowClose(WindowCloseEvent &) {
    std::cout << currentTime() << "[" << s_Application->m_Name << "] Closing window" << std::endl;
    m_Running = false;
}

void Application::OnWindowResize(WindowResizeEvent &e) {
    std::cout << currentTime() << "[" << s_Application->m_Name << "] Resizing window ["
              << e.Width() << ", " << e.Height() << "]" << std::endl;

    Renderer::OnWindowResize(e);
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
