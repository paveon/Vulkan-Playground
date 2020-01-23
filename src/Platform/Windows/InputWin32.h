#ifndef VULKAN_INPUTWIN32_H
#define VULKAN_INPUTWIN32_H

//#include <windef.h>
//#include <winuser.h>
#include "Engine/Application.h"
#include "Engine/Input.h"
#include <GLFW/glfw3.h>


class InputWin32 : public Input {
protected:
    bool impl_KeyPressed(int keycode) const override {
//       SHORT state = GetAsyncKeyState(keycode);
//       return ((1u << (sizeof(state) * 8 - 1)) & state);

       auto window = static_cast<GLFWwindow*>(Application::GetWindow().GetNativeHandle());
       int state = glfwGetKey(window, keycode);
       return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool impl_MouseButtonPressed(int button) const override {
       auto window = static_cast<GLFWwindow*>(Application::GetWindow().GetNativeHandle());
       int state = glfwGetMouseButton(window, button);
       return state == GLFW_PRESS;
    }

    float impl_MouseX() const override {
       return impl_MousePos().first;
    }

    float impl_MouseY() const override {
       return impl_MousePos().second;
    }

    std::pair<float, float> impl_MousePos() const override {
       auto window = static_cast<GLFWwindow*>(Application::GetWindow().GetNativeHandle());
       double x, y;
       glfwGetCursorPos(window, &x, &y);
       return std::make_pair(static_cast<float>(x), static_cast<float>(y));
    }
};


#endif //VULKAN_INPUTWIN32_H
