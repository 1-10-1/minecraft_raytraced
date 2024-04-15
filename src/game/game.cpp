#include "glm/trigonometric.hpp"
#include <mc/events.hpp>
#include <mc/game/game.hpp>
#include <mc/logger.hpp>
#include <mc/window.hpp>

#include <glm/vec3.hpp>

namespace game
{
    Game::Game(EventManager& eventManager, window::Window& window, Camera& camera)
        : m_window { window }, m_camera { camera }
    {
        // m_camera.lookAt({ 0.f, 0.f, 10.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f });

        eventManager.subscribe(this, &Game::onUpdate, &Game::onKeyHold, &Game::onCursorMove, &Game::onMouseButton);
    };

    void Game::onUpdate(AppUpdateEvent const& event)
    {
        m_lastDelta = event.globalTimer.getDeltaTime().count();
    };

    void Game::onMouseButton(MouseButtonEvent const& event)
    {
        if (event.action == MouseButtonEvent::Action::Pressed)
        {
            m_window.disableCursor();
        }
        else
        {
            m_window.enableCursor();
        }
    }

    void Game::onKeyHold(KeyHoldEvent const& event)
    {
        double cameraSpeed = 0.005;

        auto positive = static_cast<float>(+cameraSpeed * m_lastDelta);
        auto negative = static_cast<float>(-cameraSpeed * m_lastDelta);

        switch (event.key)
        {
            case Key::W:
                m_camera.moveZ(positive);
                break;
            case Key::S:
                m_camera.moveZ(negative);
                break;
            case Key::A:
                m_camera.moveX(negative);
                break;
            case Key::D:
                m_camera.moveX(positive);
                break;
            case Key::E:
                m_camera.moveY(positive);
                break;
            case Key::Q:
                m_camera.moveY(negative);
                break;
        }
    };

    void Game::onCursorMove(CursorMoveEvent const& event)
    {
        static bool lmbWasDown = false;
        static glm::ivec2 previousPosition {};

        if (!event.getInputManager()->isDown(MouseButton::Left))
        {
            lmbWasDown = false;
            return;
        }

        if (!lmbWasDown)
        {
            lmbWasDown       = true;
            previousPosition = event.position;
        }

        float dx = 0.1f * static_cast<float>(event.position.x - previousPosition.x);
        float dy = 0.1f * static_cast<float>(previousPosition.y - event.position.y);

        m_camera.yaw(dx);
        m_camera.pitch(dy);

        previousPosition = event.position;
    }
}  // namespace game
