#ifndef VULKAN_INPUT_MACOS_H
#define VULKAN_INPUT_MACOS_H

#include "Engine/Application.h"
#include "Engine/Input.h"
#include <GLFW/glfw3.h>


class InputMacOS : public Input {
protected:
    auto impl_KeyPressed(int keycode) const -> bool override {
//       SHORT state = GetAsyncKeyState(keycode);
//       return ((1u << (sizeof(state) * 8 - 1)) & state);

        auto *window = static_cast<GLFWwindow *>(Application::GetWindow().GetNativeHandle());
        int state = glfwGetKey(window, keycode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    auto impl_MouseButtonPressed(int button) const -> bool override {
        auto *window = static_cast<GLFWwindow *>(Application::GetWindow().GetNativeHandle());
        int state = glfwGetMouseButton(window, button);
        return state == GLFW_PRESS;
    }

    auto impl_MouseX() const -> float override {
        return impl_MousePos().first;
    }

    auto impl_MouseY() const -> float override {
        return impl_MousePos().second;
    }

    auto impl_MousePos() const -> std::pair<float, float> override {
        auto *window = static_cast<GLFWwindow *>(Application::GetWindow().GetNativeHandle());
        double pos[2] = {0};
        glfwGetCursorPos(window, &pos[0], &pos[1]);
        return std::make_pair(static_cast<float>(pos[0]), static_cast<float>(pos[1]));
    }
};


#endif //VULKAN_INPUT_MACOS_H
