#include <mc/logger.hpp>
#include <mc/window.hpp>

#include "GLFW/glfw3.h"
#include "mc/events.hpp"

namespace window
{
    Window::Window(EventManager* eventManager, glm::ivec2 resolution)
        : m_eventManager { eventManager }
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Reason: window hints need to be set before creating the window
        // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
        m_handle = glfwCreateWindow(resolution.x, resolution.y, "Minecraft", nullptr, nullptr);

        glfwSetWindowUserPointer(m_handle, this);

        if (glfwRawMouseMotionSupported() != 0)
        {
            logger::info("Raw mouse motion supported. Enabling it.");
            glfwSetInputMode(m_handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        glfwSetKeyCallback(m_handle, keyCallback);
        glfwSetWindowPosCallback(m_handle, windowPositionCallback);
        glfwSetWindowSizeCallback(m_handle, windowSizeCallback);
        glfwSetFramebufferSizeCallback(m_handle, framebufferSizeCallback);
        glfwSetWindowCloseCallback(m_handle, windowCloseCallback);
        glfwSetWindowRefreshCallback(m_handle, windowRefreshCallback);
        glfwSetWindowFocusCallback(m_handle, windowFocusCallback);
        glfwSetWindowIconifyCallback(m_handle, windowMinimizeCallback);
        glfwSetWindowMaximizeCallback(m_handle, windowMaximizeCallback);
        glfwSetMouseButtonCallback(m_handle, mouseButtonCallback);
        glfwSetCursorPosCallback(m_handle, cursorPositionCallback);
        glfwSetCursorEnterCallback(m_handle, cursorEnterCallback);
        glfwSetScrollCallback(m_handle, scrollCallback);
        glfwSetDropCallback(m_handle, dropCallback);
    };

    Window::Window(Window&& window) noexcept
        : m_eventManager { window.m_eventManager }, m_handle(window.m_handle)
    {
    }

    Window::~Window()
    {
        if (m_handle == nullptr)
        {
            return;  // Window was copied/moved
        }

        glfwDestroyWindow(m_handle);
        glfwTerminate();
    }

    void Window::prepare()
    {
        m_eventManager->dispatchEvent(WindowRefreshEvent {});
    }

    void Window::pollEvents()
    {
        glfwPollEvents();
    }

    void Window::keyCallback(GLFWwindow* window, int keyid, int /*scancode*/, int action, int mods)
    {
        auto* self                 = static_cast<Window*>(glfwGetWindowUserPointer(window));
        auto key                   = static_cast<Key>(keyid);
        InputManager& inputManager = self->m_inputManager;

        // Todo: make a single event called KeyEvent and send them an action parameter
        switch (action)
        {
            case GLFW_PRESS:
                inputManager.setKey(key, true);
                self->m_eventManager->dispatchEvent(KeyPressEvent(&inputManager, key, mods, false));
                break;
            case GLFW_REPEAT:
                inputManager.setKey(key, true);
                self->m_eventManager->dispatchEvent(KeyPressEvent(&inputManager, key, mods, true));
                break;
            case GLFW_RELEASE:
                inputManager.setKey(key, false);
                self->m_eventManager->dispatchEvent(KeyReleaseEvent(&inputManager, key, mods));
                break;
            default:
                break;
        }
    }

    void Window::windowPositionCallback(GLFWwindow* window, int x, int y)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(WindowMoveEvent({ x, y }));
    }

    void Window::windowSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(WindowResizeEvent({ width, height }));
    }

    void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(WindowFramebufferResizeEvent({ width, height }));
    }

    void Window::windowCloseCallback(GLFWwindow* window)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(WindowCloseEvent());

        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    void Window::windowRefreshCallback(GLFWwindow* window)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(WindowRefreshEvent());
    }

    void Window::windowFocusCallback(GLFWwindow* window, int focused)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(WindowFocusChangedEvent(
            focused == 0 ? WindowFocusChangedEvent::Defocused : WindowFocusChangedEvent::Focused));
    }

    void Window::windowMinimizeCallback(GLFWwindow* window, int iconified)
    {
        if (iconified == 0)
        {
            return;
        }

        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(
            WindowMinOrMaximizeEvent(WindowMinOrMaximizeEvent::Minimized));
    }

    void Window::windowMaximizeCallback(GLFWwindow* window, int maximized)
    {
        if (maximized == 0)
        {
            return;
        }

        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(
            WindowMinOrMaximizeEvent(WindowMinOrMaximizeEvent::Maximized));
    }

    void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        MouseButton mousebutton = glfwIntToMouseButton(button);
        auto buttonaction       = static_cast<MouseButtonEvent::Action>(action);

        self->m_eventManager->dispatchEvent(
            MouseButtonEvent(&self->m_inputManager, mousebutton, buttonaction, mods));

        switch (buttonaction)
        {
            case (MouseButtonEvent::Action::Pressed):
                self->m_inputManager.setButton(mousebutton, true);
                break;
            case (MouseButtonEvent::Action::Released):
                self->m_inputManager.setButton(mousebutton, false);
                break;
        }
    }

    void Window::cursorPositionCallback(GLFWwindow* window, double x, double y)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_inputManager.setCursorPos({ x, y });

        self->m_eventManager->dispatchEvent(CursorMoveEvent(&self->m_inputManager, { x, y }));
    }

    void Window::cursorEnterCallback(GLFWwindow* window, int entered)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(CursorFocusChangedEvent(
            entered != 0 ? CursorFocusChangedEvent::Focused : CursorFocusChangedEvent::Defocused));
    }

    void Window::scrollCallback(GLFWwindow* window, double x, double y)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(MouseScrollEvent(&self->m_inputManager, { x, y }));
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    void Window::dropCallback(GLFWwindow* window, int count, char const* paths[])
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        self->m_eventManager->dispatchEvent(
            WindowDragAndDropEvent(static_cast<size_t>(count), paths));
    }

}  // namespace window
