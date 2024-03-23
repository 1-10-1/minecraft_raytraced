#include "mc/window.hpp"
#include <mc/game/game.hpp>
#include <mc/logger.hpp>

namespace game
{
    Game::Game()
        : m_window {
              {800, 600}
    } {};
}  // namespace game
