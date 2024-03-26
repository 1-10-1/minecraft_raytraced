#pragma once

#include "../renderer/renderer.hpp"
#include "../window.hpp"

namespace game
{
    class Game
    {
    public:
        Game();

        void runLoop();

    private:
        window::Window m_window;
        renderer::Renderer m_renderer;
    };
}  // namespace game
