#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include <iostream>
#include <Renderer/renderer.h>
#include <mutex>
#include <queue>
#include "Events/Event.h"
#include "Engine/Events/WindowEvents.h"
#include "Engine/Events/MouseEvents.h"
#include "Engine/Events/KeyEvents.h"
#include "Window.h"
#include "LayerStack.h"

class Application {
private:
    const uint32_t WindowWidth = 800;
    const uint32_t WindowHeight = 600;

    Instance m_Instance;
    std::shared_ptr<Window> m_Window = nullptr;
    Renderer m_Renderer;

    std::queue<std::unique_ptr<Event>> m_WindowEventQueue;
    std::mutex m_WindowEventMutex;

    LayerStack m_LayerStack;
    bool m_Running;

    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);
    bool OnMouseMove(MouseMoveEvent&) { return true; };
    bool OnMouseButtonPress(MouseButtonPressEvent&) { return true; };
    bool OnMouseButtonRelease(MouseButtonReleaseEvent&) { return true; };
    bool OnMouseScroll(MouseScrollEvent&) { return true; };
    bool OnKeyPress(KeyPressEvent&) { return true; };
    bool OnKeyRelease(KeyReleaseEvent&) { return true; };

    void ProcessEventQueue();

protected:
    Application();

    void OnEvent(std::unique_ptr<Event> e);

    const Layer* PushLayer(std::unique_ptr<Layer> layer) { return m_LayerStack.PushLayer(std::move(layer)); }
    const Layer* PushOverlay(std::unique_ptr<Layer> overlay) { return m_LayerStack.PushOverlay(std::move(overlay)); }
    std::unique_ptr<Layer> PopLayer(const Layer* layer) { return m_LayerStack.PopLayer(layer); }
    std::unique_ptr<Layer> PopOverlay(const Layer* overlay) { return m_LayerStack.PopOverlay(overlay); }

public:
    virtual ~Application();

    void Run();
};


#endif //VULKAN_APPLICATION_H
