#include "mc/window.hpp"
#include <mc/game/game.hpp>
#include <mc/logger.hpp>

namespace game
{
    Game::Game()
        : m_window { window::Window({ 800, 600 }) },
          m_renderer { renderer::Renderer(m_window.getHandle()) } {};
}  // namespace game
