#ifndef VULKAN_WINDOWEVENTS_H
#define VULKAN_WINDOWEVENTS_H

#include <sstream>
#include "Event.h"


class WindowMoveEvent : public Event {
public:
    WindowMoveEvent() = default;

    EventType Type() const override { return EventType::WindowMove; }

    const char* GetName() const override { return "WindowMoveEvent"; }

    uint32_t CategoryFlags() const override { return EventCategory::Window; }
};


class WindowCloseEvent : public Event {
public:
    WindowCloseEvent() = default;

    EventType Type() const override { return EventType::WindowClose; }

    const char* GetName() const override { return "WindowCloseEvent"; }

    uint32_t CategoryFlags() const override { return EventCategory::Window; }
};


class WindowResizeEvent : public Event {
private:
    uint32_t m_Width;
    uint32_t m_Height;

public:
    WindowResizeEvent(uint32_t width, uint32_t height) : m_Width(width), m_Height(height) {}

    EventType Type() const override { return EventType::WindowResize; }

    const char* GetName() const override { return "WindowResizeEvent"; }

    uint32_t CategoryFlags() const override { return EventCategory::Window; }

    uint32_t Width() const { return m_Width; }

    uint32_t Height() const { return m_Height; }
};

#endif //VULKAN_WINDOWEVENTS_H
