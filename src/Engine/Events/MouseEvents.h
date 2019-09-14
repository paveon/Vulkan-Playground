#ifndef VULKAN_MOUSEEVENTS_H
#define VULKAN_MOUSEEVENTS_H

#include <sstream>
#include "Event.h"

class MouseMoveEvent : public Event {
protected:
    double m_X;
    double m_Y;

public:
    MouseMoveEvent(double x, double y) : m_X(x), m_Y(y) {}

    EventType Type() const override { return EventType::MouseMove; }

    const char* GetName() const override { return "MouseMoveEvent"; }

    uint32_t CategoryFlags() const override { return EventCategory::Input | EventCategory::Mouse; }

    std::string ToString() const override {
       std::ostringstream ss;
       ss << GetName() << ": [" << m_X << ", " << m_Y << "]";
       return ss.str();
    }
};


class MouseScrollEvent : public Event {
protected:
    double m_OffsetX;
    double m_OffsetY;

public:
    MouseScrollEvent(double offsetX, double offsetY) : m_OffsetX(offsetX), m_OffsetY(offsetY) {}

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
