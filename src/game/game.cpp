#include <mc/events.hpp>
#include <mc/game/game.hpp>
#include <mc/logger.hpp>
#include <mc/window.hpp>

namespace game
{
    Game::Game() = default;

    void onUpdate(AppUpdateEvent const& /* unused */) {};
}  // namespace game
