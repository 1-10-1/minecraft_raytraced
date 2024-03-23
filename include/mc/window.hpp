#pragma once

#include <GLFW/glfw3.h>
#include <glm/ext/vector_int2.hpp>

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

        void pollEvents();

    private:
        bool m_shouldClose {};

        GLFWwindow* m_handle {};
    };
}  // namespace window
