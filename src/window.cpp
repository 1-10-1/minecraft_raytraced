#include <mc/logger.hpp>
#include <mc/window.hpp>

#include "GLFW/glfw3.h"

namespace window
{
    Window::Window(glm::ivec2 resolution)
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Reason: window hints need to be set before creating the window
        // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
        m_handle = glfwCreateWindow(resolution.x, resolution.y, "Minecraft", nullptr, nullptr);
    };

    Window::Window(Window&& window) noexcept : m_handle(window.m_handle) {}

    Window::~Window()
    {
        if (m_handle == nullptr)
        {
            return;  // Window was copied/moved
        }

        glfwDestroyWindow(m_handle);
        glfwTerminate();
    }

    void Window::pollEvents() {}

}  // namespace window
