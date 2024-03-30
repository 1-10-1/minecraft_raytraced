#include <mc/events.hpp>
#include <mc/game/game.hpp>
#include <mc/logger.hpp>
#include <mc/window.hpp>

namespace game
{

    Game::Game()
        : m_window { window::Window(&m_eventManager, { 800, 600 }) },
          m_renderer { renderer::Renderer(
              m_eventManager, m_window.getHandle(), m_window.getDimensions()) } {};
}  // namespace game
