#pragma once

#include "../camera.hpp"
#include "../event_manager.hpp"
#include "../events.hpp"
#include "../window.hpp"

namespace game
{
    class Game
    {
    public:
        explicit Game(EventManager& eventManager, window::Window& window, Camera& camera);

        void onUpdate(AppUpdateEvent const& event);
        void onKeyHold(KeyHoldEvent const& event);
        void onCursorMove(CursorMoveEvent const& event);
        void onMouseButton(MouseButtonEvent const& event);

    private:
        window::Window& m_window;
        Camera& m_camera;

        double m_lastDelta {};
    };
}  // namespace game
