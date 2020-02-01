#ifndef VULKAN_EVENT_H
#define VULKAN_EVENT_H

#include <string>
#include <cstdint>

enum class EventType {
    None,
    WindowMove,
    WindowClose,
    WindowResize,
    KeyPress,
    KeyRelease,
    MouseMove,
    MouseButtonPress,
    MouseButtonRelease,
    MouseScroll,
    CharacterPress
};


namespace EventCategory {
    enum : uint32_t {
        None = 0x00,
        Window = 0x01,
        Input = 0x02,
        Keyboard = 0x04,
        Mouse = 0x08,
        MouseButton = 0x10
    };
}


class Event {
protected:
    Event() = default;

public:
    virtual uint32_t CategoryFlags() const = 0;
    virtual EventType Type() const = 0;
    virtual const char* GetName() const = 0;
    virtual std::string ToString() const { return GetName(); }

    bool HasCategories(uint32_t categoryFlags) { return CategoryFlags() &categoryFlags; }

    bool Handled = false;
};


inline std::ostream& operator<<(std::ostream& out, const Event& e) {
    out << e.ToString();
    return out;
}

#endif //VULKAN_EVENT_H
