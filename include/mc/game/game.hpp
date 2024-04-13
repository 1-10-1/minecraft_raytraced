#pragma once

#include "../events.hpp"

namespace game
{
    class Game
    {
    public:
        Game();

        void onUpdate(AppUpdateEvent const& event);

    private:
    };
}  // namespace game
