#pragma once

#include "../event_manager.hpp"
#include "../renderer/renderer.hpp"
#include "../window.hpp"

namespace game
{
    class Game
    {
    public:
        Game();

        void runLoop();

        void onCursorMove(CursorMoveEvent const& event);

    private:
        EventManager m_eventManager {};
        window::Window m_window;
        renderer::Renderer m_renderer;
    };
}  // namespace game
