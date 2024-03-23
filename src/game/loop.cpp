#include <mc/game/game.hpp>
#include <mc/game/loop.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/render.hpp>

#include <fmt/printf.h>

namespace game
{
    void Game::runLoop()
    {
        while (!m_window.shouldClose())
        {
            m_window.pollEvents();
        };
    }
}  // namespace game
