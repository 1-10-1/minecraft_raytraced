#pragma once

#include "../assetloader.hpp"
#include "../camera.hpp"
#include "../event_manager.hpp"
#include "./backend/renderer_backend.hpp"

#include "mc/events.hpp"

namespace renderer
{
    class Renderer
    {
    public:
        explicit Renderer(EventManager& eventManager, window::Window& window, Camera& camera);

        void onRender(AppRenderEvent const& /* unused */);
        void onUpdate(AppUpdateEvent const& /* unused */);
        void onKeyPress(KeyPressEvent const& event);
        void onFramebufferResize(WindowFramebufferResizeEvent const& event);

    private:
        Camera& m_camera;

        Model m_model;

        backend::RendererBackend m_backend;
    };
}  // namespace renderer
