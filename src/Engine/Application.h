#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include <iostream>
#include <mutex>
#include <queue>

#include "Renderer/renderer.h"
#include "Events/Event.h"
#include "Events/WindowEvents.h"
#include "Events/MouseEvents.h"
#include "Events/KeyEvents.h"
#include "Window.h"
#include "LayerStack.h"

class Application {
private:
    static Application* s_Application;

    const std::string m_ApplicationName;

    std::unique_ptr<Window> m_Window = nullptr;

    std::queue<std::unique_ptr<Event>> m_WindowEventQueue;
    std::mutex m_WindowEventMutex;

    LayerStack m_LayerStack;
    bool m_Running = false;

    void Init();

    bool OnWindowClose(WindowCloseEvent& e);

    bool OnWindowResize(WindowResizeEvent& e);

    bool OnMouseButtonPress(MouseButtonPressEvent&) { return true; }

    bool OnMouseButtonRelease(MouseButtonReleaseEvent&) { return true; }

    bool OnMouseScroll(MouseScrollEvent&) { return true; }

    bool OnKeyPress(KeyPressEvent&) { return true; }

    bool OnKeyRelease(KeyReleaseEvent&) { return true; }

    bool OnMouseMove(MouseMoveEvent&) { return true; }

    void ProcessEventQueue();

protected:
    explicit Application(const char* name);

    void OnEvent(std::unique_ptr<Event> e);

    const Layer* PushLayer(std::unique_ptr<Layer> layer) { return m_LayerStack.PushLayer(std::move(layer)); }

    const Layer* PushOverlay(std::unique_ptr<Layer> overlay) { return m_LayerStack.PushOverlay(std::move(overlay)); }

    std::unique_ptr<Layer> PopLayer(const Layer* layer) { return m_LayerStack.PopLayer(layer); }

    std::unique_ptr<Layer> PopOverlay(const Layer* overlay) { return m_LayerStack.PopOverlay(overlay); }

    std::unique_ptr<Renderer> m_Renderer = nullptr;

public:
    virtual ~Application();

    static Application& Get() { return *s_Application; }

    void Run();

    static Window& GetWindow() { return *s_Application->m_Window; }

    static GfxContext& GetGraphicsContext() { return s_Application->m_Window->Context(); }

    static std::unique_ptr<Application> CreateApplication();
};


#endif //VULKAN_APPLICATION_H
