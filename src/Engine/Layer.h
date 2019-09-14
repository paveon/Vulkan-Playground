#ifndef VULKAN_LAYER_H
#define VULKAN_LAYER_H

#include <string>
#include <Engine/Events/Event.h>
#include <Engine/Events/WindowEvents.h>
#include <Engine/Events/MouseEvents.h>
#include <Engine/Events/KeyEvents.h>

class Layer {
protected:
    std::string m_DebugName;

public:
    explicit Layer(const char* name = "Layer") : m_DebugName(name) {}
    virtual ~Layer() = default;

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate() {}
    virtual void OnEvent(Event&) {};
    virtual bool OnMouseMove(MouseMoveEvent&) { return true; };
    virtual bool OnMouseButtonPress(MouseButtonPressEvent&) { return true; };
    virtual bool OnMouseButtonRelease(MouseButtonReleaseEvent&) { return true; };
    virtual bool OnMouseScroll(MouseScrollEvent&) { return true; };
    virtual bool OnKeyPress(KeyPressEvent&) { return true; };
    virtual bool OnKeyRelease(KeyReleaseEvent&) { return true; };

    const std::string& LayerName() const { return m_DebugName; }
};

#endif //VULKAN_LAYER_H
