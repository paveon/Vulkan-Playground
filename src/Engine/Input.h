#ifndef VULKAN_INPUT_H
#define VULKAN_INPUT_H

#include <memory>

class Input {
private:
    static std::unique_ptr<Input> s_Instance;

protected:
    virtual auto impl_KeyPressed(int keycode) const -> bool = 0;

    virtual auto impl_MouseButtonPressed(int button) const -> bool = 0;

    virtual auto impl_MouseX() const -> float = 0;

    virtual auto impl_MouseY() const -> float = 0;

    virtual auto impl_MousePos() const -> std::pair<float, float> = 0;

public:
    static void InitInputSubsystem();

    virtual ~Input() = default;

    static auto KeyPressed(int keycode) -> bool { return s_Instance->impl_KeyPressed(keycode); }

    static auto MouseButtonPressed(int button) -> bool { return s_Instance->impl_MouseButtonPressed(button); }

    static auto MouseX() -> float { return s_Instance->impl_MouseX(); }

    static auto MouseY() -> float { return s_Instance->impl_MouseY(); }

    static auto MousePos() -> std::pair<float, float> { return s_Instance->impl_MousePos(); }
};

#endif //VULKAN_INPUT_H
