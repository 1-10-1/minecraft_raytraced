#pragma once

#include <GLFW/glfw3.h>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_uint2.hpp>

#include "event_queue.hpp"
#include "events.hpp"

namespace window
{
    class Window
    {
    public:
        explicit Window(glm::ivec2 resolution);

        ~Window();

        Window(Window&&) noexcept;

        Window(Window const&)                    = default;
        auto operator=(Window const&) -> Window& = default;
        auto operator=(Window&&) -> Window&      = default;

        [[nodiscard]] auto shouldClose() const -> bool { return m_shouldClose; }

        [[nodiscard]] auto getHandle() const -> GLFWwindow* { return m_handle; }

        void pollEvents();

    private:
        bool m_shouldClose {};

        GLFWwindow* m_handle {};
    };
}  // namespace window
