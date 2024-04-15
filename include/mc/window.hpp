#pragma once

#include <GLFW/glfw3.h>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_uint2.hpp>

#include "event_manager.hpp"

namespace window
{
    class Window
    {
    public:
        explicit Window(EventManager& eventManager);

        ~Window();

        Window(Window&&) noexcept;

        Window(Window const&)                    = delete;
        auto operator=(Window const&) -> Window& = delete;
        auto operator=(Window&&) -> Window&      = delete;

        explicit operator GLFWwindow*() { return m_handle; }

        [[nodiscard]] auto shouldClose() const -> bool
        {
            return m_shouldClose || (glfwWindowShouldClose(m_handle) != 0);
        }

        [[nodiscard]] auto getHandle() const -> GLFWwindow* { return m_handle; }

        [[nodiscard]] auto getWindowDimensions() const -> glm::uvec2 { return m_windowDimensions; }

        [[nodiscard]] auto getFramebufferDimensions() const -> glm::uvec2 { return m_framebufferDimensions; }

        static void pollEvents();

        static void keyCallback(GLFWwindow* window, int keyid, int scancode, int action, int mods);

        static void windowPositionCallback(GLFWwindow* window, int x, int y);

        static void windowSizeCallback(GLFWwindow* window, int width, int height);

        static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

        static void windowCloseCallback(GLFWwindow* window);

        static void windowRefreshCallback(GLFWwindow* window);

        static void windowFocusCallback(GLFWwindow* window, int focused);

        static void windowMinimizeCallback(GLFWwindow* window, int iconified);

        static void windowMaximizeCallback(GLFWwindow* window, int maximized);

        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

        static void cursorPositionCallback(GLFWwindow* window, double x, double y);

        static void cursorEnterCallback(GLFWwindow* window, int entered);

        static void scrollCallback(GLFWwindow* window, double x, double y);

        // Reason: This function signature cannot be changed because it is passed directly to GLFW
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
        static void dropCallback(GLFWwindow* window, int count, char const* paths[]);

        void onUpdate(AppUpdateEvent const& event);

        void disableCursor()
        {
            m_cursorDisabled = true;
            glfwSetInputMode(m_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        void enableCursor()
        {
            m_cursorDisabled = false;
            glfwSetInputMode(m_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        void toggleCursor() { m_cursorDisabled ? enableCursor() : disableCursor(); }

    private:
        bool m_shouldClose {};

        InputManager m_inputManager {};
        EventManager& m_eventManager;
        GLFWwindow* m_handle {};

        glm::uvec2 m_framebufferDimensions {};
        glm::uvec2 m_windowDimensions {};

        bool m_cursorDisabled { false };
    };
}  // namespace window
