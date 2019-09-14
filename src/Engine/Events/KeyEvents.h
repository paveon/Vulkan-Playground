#ifndef VULKAN_KEYEVENTS_H
#define VULKAN_KEYEVENTS_H

#include <sstream>
#include "Event.h"

class KeyEvent : public Event {
protected:
    int m_KeyCode;

    explicit KeyEvent(int keyCode) : m_KeyCode(keyCode) {}

public:
    uint32_t CategoryFlags() const override { return EventCategory::Input | EventCategory::Keyboard; }

    std::string ToString() const override {
       std::ostringstream ss;
       ss << GetName() << ": ";
       if (m_KeyCode < 256) ss << static_cast<char>(m_KeyCode);
       else ss << m_KeyCode;
       return ss.str();
    }
};


class KeyPressEvent : public KeyEvent {
    uint32_t m_RepeatCount = 0;

public:
    KeyPressEvent(int keyCode, int repeatCount) : KeyEvent(keyCode), m_RepeatCount(repeatCount) {}

    EventType Type() const override { return EventType::KeyPress; }

    const char* GetName() const override { return "KeyPressEvent"; }

    std::string ToString() const override {
       std::ostringstream ss;
       ss << GetName() << ": ";
       if (m_KeyCode < 256) ss << static_cast<char>(m_KeyCode);
       else ss << m_KeyCode;

       ss << "(" << m_RepeatCount << ")";
       return ss.str();
    }

    uint32_t RepeatCount() const { return m_RepeatCount; }
};


class KeyReleaseEvent : public KeyEvent {
public:
    explicit KeyReleaseEvent(int keyCode) : KeyEvent(keyCode) {}

    EventType Type() const override { return EventType::KeyRelease; }

    const char* GetName() const override { return "KeyReleaseEvent"; }
};

#endif //VULKAN_KEYEVENTS_H
