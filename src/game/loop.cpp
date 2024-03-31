#include "mc/events.hpp"
#include <mc/game/game.hpp>
#include <mc/game/loop.hpp>
#include <mc/logger.hpp>

#include <fmt/printf.h>

namespace game
{
    void Game::runLoop()
    {
        m_window.prepare();

        while (!m_window.shouldClose())
        {
            m_eventManager.dispatchEvent(AppRenderEvent {});

            window::Window::pollEvents();
        };
    }
}  // namespace game
