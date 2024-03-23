#pragma once

#include <GLFW/glfw3.h>

enum class MouseButton
{
    Left         = GLFW_MOUSE_BUTTON_LEFT,
    Right        = GLFW_MOUSE_BUTTON_RIGHT,
    Middle       = GLFW_MOUSE_BUTTON_MIDDLE,
    MouseButton4 = GLFW_MOUSE_BUTTON_4,
    MouseButton5 = GLFW_MOUSE_BUTTON_5,
    MouseButton6 = GLFW_MOUSE_BUTTON_6,
    MouseButton7 = GLFW_MOUSE_BUTTON_7,
    MouseButton8 = GLFW_MOUSE_BUTTON_8
};

inline auto glfwIntToMouseButton(int button) -> MouseButton
{
    switch (button)
    {
        case (GLFW_MOUSE_BUTTON_LEFT):
            return MouseButton::Left;
        case (GLFW_MOUSE_BUTTON_RIGHT):
            return MouseButton::Right;
        case (GLFW_MOUSE_BUTTON_MIDDLE):
            return MouseButton::Middle;
        case (GLFW_MOUSE_BUTTON_4):
            return MouseButton::MouseButton4;
        case (GLFW_MOUSE_BUTTON_5):
            return MouseButton::MouseButton5;
        case (GLFW_MOUSE_BUTTON_6):
            return MouseButton::MouseButton6;
        case (GLFW_MOUSE_BUTTON_7):
            return MouseButton::MouseButton7;
        case (GLFW_MOUSE_BUTTON_8):
            return MouseButton::MouseButton8;
    }

#pragma GCC diagnostic ignored "-Wreturn-type"
}
