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
    static Application *s_Application;

    const std::string m_ApplicationName;

    std::unique_ptr<Window> m_Window = nullptr;

    std::queue<std::unique_ptr<Event>> m_WindowEventQueue;
    std::mutex m_WindowEventMutex;

    LayerStack m_LayerStack;
    bool m_Running = false;

    void Init();

    void OnEvent(std::unique_ptr<Event> e);

    auto OnWindowClose(WindowCloseEvent &e) -> bool;

    auto OnWindowResize(WindowResizeEvent &e) -> bool;

    auto OnMouseButtonPress(MouseButtonPressEvent &) -> bool { return true; }

    auto OnMouseButtonRelease(MouseButtonReleaseEvent &) -> bool { return true; }

    auto OnMouseScroll(MouseScrollEvent &) -> bool { return true; }

    auto OnKeyPress(KeyPressEvent &) -> bool { return true; }

    auto OnKeyRelease(KeyReleaseEvent &) -> bool { return true; }

    auto OnMouseMove(MouseMoveEvent &) -> bool { return true; }

    auto OnCharacterPress(CharacterPressEvent &) -> bool { return true; }

    void ProcessEventQueue();

protected:
    explicit Application(const char *name);

    auto PushLayer(std::unique_ptr<Layer> layer) -> Layer * { return m_LayerStack.PushLayer(std::move(layer)); }

    auto PushOverlay(std::unique_ptr<Layer> overlay) -> Layer * { return m_LayerStack.PushOverlay(std::move(overlay)); }

    auto PopLayer(const Layer *layer) -> std::unique_ptr<Layer> { return m_LayerStack.PopLayer(layer); }

    auto PopOverlay(const Layer *overlay) -> std::unique_ptr<Layer> { return m_LayerStack.PopOverlay(overlay); }

    std::unique_ptr<Renderer> m_Renderer = nullptr;

public:
    virtual ~Application();

    static auto Get() -> Application & { return *s_Application; }

    static auto GetWindow() -> Window & { return *s_Application->m_Window; }

    static auto GetGraphicsContext() -> GfxContext & { return s_Application->m_Window->Context(); }

    static auto CreateApplication() -> std::unique_ptr<Application>;

    void Run();
};


#endif //VULKAN_APPLICATION_H
