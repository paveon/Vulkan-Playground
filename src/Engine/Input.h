#ifndef VULKAN_INPUT_H
#define VULKAN_INPUT_H

#include <memory>

class Input {
private:
    static std::unique_ptr<Input> s_Instance;

protected:
    virtual bool impl_KeyPressed(int keycode) const = 0;

    virtual bool impl_MouseButtonPressed(int button) const = 0;

    virtual float impl_MouseX() const = 0;

    virtual float impl_MouseY() const = 0;

    virtual std::pair<float, float> impl_MousePos() const = 0;

public:
   static void InitInputSubsystem();

    static bool KeyPressed(int keycode) { return s_Instance->impl_KeyPressed(keycode); }

    static bool MouseButtonPressed(int button) { return s_Instance->impl_MouseButtonPressed(button); }

    static float MouseX() { return s_Instance->impl_MouseX(); }

    static float MouseY() { return s_Instance->impl_MouseY(); }

    static std::pair<float, float> MousePos() { return s_Instance->impl_MousePos(); }
};

#endif //VULKAN_INPUT_H
