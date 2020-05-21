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
    virtual ~Event() = default;

    virtual auto CategoryFlags() const -> uint32_t = 0;

    virtual auto Type() const -> EventType = 0;

    virtual auto GetName() const -> const char * = 0;

    virtual auto ToString() const -> std::string { return GetName(); }

    auto HasCategories(uint32_t categoryFlags) const -> bool { return CategoryFlags() & categoryFlags; }

    bool Handled = false;
};


inline auto operator<<(std::ostream &out, const Event &e) -> std::ostream & {
    out << e.ToString();
    return out;
}

#endif //VULKAN_EVENT_H
