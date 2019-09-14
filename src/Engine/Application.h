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

class Application {
private:
    const uint32_t WindowWidth = 800;
    const uint32_t WindowHeight = 600;

    Instance m_Instance;
    std::shared_ptr<Window> m_Window = nullptr;
    Renderer m_Renderer;
    std::queue<std::unique_ptr<Event>> m_WindowEventQueue;
    mutable std::mutex m_WindowEventMutex;
    bool m_Running;

    bool OnWindowClose(const WindowCloseEvent& e);
    bool OnWindowResize(const WindowResizeEvent& e);
    virtual bool OnMouseMove(const MouseMoveEvent&) { return true; };
    virtual bool OnMouseButtonPress(const MouseButtonPressEvent&) { return true; };
    virtual bool OnMouseButtonRelease(const MouseButtonReleaseEvent&) { return true; };
    virtual bool OnMouseScroll(const MouseScrollEvent&) { return true; };
    virtual bool OnKeyPress(const KeyPressEvent&) { return true; };
    virtual bool OnKeyRelease(const KeyReleaseEvent&) { return true; };

    void ProcessEventQueue();

protected:
    Application();

    void OnEvent(std::unique_ptr<Event> e);

public:
    virtual ~Application();

    void Run();
};


#endif //VULKAN_APPLICATION_H
