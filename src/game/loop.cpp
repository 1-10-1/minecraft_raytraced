#include "mc/events.hpp"
#include <mc/game/game.hpp>
#include <mc/game/loop.hpp>
#include <mc/logger.hpp>

#include <fmt/printf.h>
#include <tracy/Tracy.hpp>

namespace game
{
    void Game::runLoop()
    {
        m_window.prepare();

        while (!m_window.shouldClose())
        {
            window::Window::pollEvents();

            m_eventManager.dispatchEvent(AppRenderEvent {});

            FrameMark;
        };
    }
}  // namespace game
