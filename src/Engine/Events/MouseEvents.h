#ifndef VULKAN_MOUSEEVENTS_H
#define VULKAN_MOUSEEVENTS_H

#include <sstream>
#include "Event.h"

class MouseMoveEvent : public Event {
protected:
    float m_X;
    float m_Y;

public:
    MouseMoveEvent(float x, float y) : m_X(x), m_Y(y) {}

    EventType Type() const override { return EventType::MouseMove; }

    const char* GetName() const override { return "MouseMoveEvent"; }

    uint32_t CategoryFlags() const override { return EventCategory::Input | EventCategory::Mouse; }

    std::string ToString() const override {
       std::ostringstream ss;
       ss << GetName() << ": [" << m_X << ", " << m_Y << "]";
       return ss.str();
    }

    float X() const { return m_X; }
    float Y() const { return m_Y; }
};


class MouseScrollEvent : public Event {
protected:
    float m_OffsetX;
    float m_OffsetY;

public:
    MouseScrollEvent(float offsetX, float offsetY) : m_OffsetX(offsetX), m_OffsetY(offsetY) {}

    EventType Type() const override { return EventType::MouseScroll; }

    const char* GetName() const override { return "MouseScrollEvent"; }

    uint32_t CategoryFlags() const override { return EventCategory::Input | EventCategory::Mouse; }

    std::string ToString() const override {
       std::ostringstream ss;
       ss << GetName() << ": [" << m_OffsetX << ", " << m_OffsetY << "]";
       return ss.str();
    }
};


class MouseButtonEvent : public Event {
protected:
    int m_Button;

    explicit MouseButtonEvent(int button) : m_Button(button) {}

public:
    uint32_t CategoryFlags() const override { return EventCategory::Input | EventCategory::Mouse | EventCategory::MouseButton; }

    std::string ToString() const override {
       std::ostringstream ss;
       ss << GetName() << ": " << m_Button;
       return ss.str();
    }

    int Button() const { return m_Button; }
};


class MouseButtonPressEvent : public MouseButtonEvent {
public:
    explicit MouseButtonPressEvent(int button) : MouseButtonEvent(button) {}

    EventType Type() const override { return EventType::MouseButtonPress; }

    const char* GetName() const override { return "MouseButtonPressEvent"; }
};


class MouseButtonReleaseEvent : public MouseButtonEvent {
public:
    explicit MouseButtonReleaseEvent(int button) : MouseButtonEvent(button) {}

    EventType Type() const override { return EventType::MouseButtonRelease; }

    const char* GetName() const override { return "MouseButtonReleaseEvent"; }
};

#endif //VULKAN_MOUSEEVENTS_H
