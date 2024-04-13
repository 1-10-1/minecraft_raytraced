#include <mc/events.hpp>
#include <mc/game/game.hpp>
#include <mc/logger.hpp>
#include <mc/window.hpp>

namespace game
{
    Game::Game()
        : m_window { window::Window(&m_eventManager) }, m_renderer { renderer::Renderer(m_eventManager, m_window) }
    {
        m_eventManager.subscribe(this, &Game::onCursorMove);
    };

    void Game::onCursorMove(CursorMoveEvent const& event)
    {
        static int count = 0;
        ++count;

        if (count == 1000)
        {
            logger::debug("onCursorMove event detached");
            // m_eventManager.unsubscribe(this, &Game::onCursorMove);
        }

        if (count > 1000)
        {
            return;
        }

        logger::debug("[{}] cursor moved to position ({}, {})", count, event.position.x, event.position.y);
    }
}  // namespace game
