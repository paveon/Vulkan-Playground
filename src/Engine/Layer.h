#ifndef VULKAN_LAYER_H
#define VULKAN_LAYER_H

#include <iostream>
#include <string>
#include "Events/Event.h"
#include "Events/WindowEvents.h"
#include "Events/MouseEvents.h"
#include "Events/KeyEvents.h"

class Layer {
protected:
    std::string m_DebugName;

public:
    explicit Layer(const char* name = "Layer") : m_DebugName(name) {}
    virtual ~Layer() { std::cout << "[Application] " << m_DebugName << "::Destroyed" << std::endl; };

    virtual void OnAttach() { std::cout << "[Application] " << m_DebugName << "::Attached" << std::endl; }
    virtual void OnDetach() { std::cout << "[Application] " << m_DebugName << "::Detached" << std::endl; }
    virtual void OnUpdate(uint32_t) {}
    virtual auto OnDraw(uint32_t) -> VkCommandBuffer { return nullptr; };

    virtual void OnEvent(Event&) {}
    virtual auto OnWindowResize(WindowResizeEvent&) -> bool { return true; }
    virtual auto OnMouseMove(MouseMoveEvent&) -> bool { return true; };
    virtual auto OnMouseButtonPress(MouseButtonPressEvent&) -> bool { return true; }
    virtual auto OnMouseButtonRelease(MouseButtonReleaseEvent&) -> bool { return true; }
    virtual auto OnMouseScroll(MouseScrollEvent&) -> bool { return true; }
    virtual auto OnKeyPress(KeyPressEvent&) -> bool { return true; }
    virtual auto OnKeyRelease(KeyReleaseEvent&) -> bool { return true; }
    virtual auto OnCharacterPress(CharacterPressEvent&) -> bool { return true; }

    auto LayerName() const -> const std::string& { return m_DebugName; }
};

#endif //VULKAN_LAYER_H
