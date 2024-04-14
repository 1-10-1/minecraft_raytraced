#pragma once

#include "../event_manager.hpp"
#include "./backend/renderer_backend.hpp"

#include "mc/events.hpp"

namespace renderer
{
    class Renderer
    {
    public:
        explicit Renderer(EventManager& eventManager, window::Window& window);

        void onRender(AppRenderEvent const& /* unused */);
        void onUpdate(AppUpdateEvent const& /* unused */);
        void onFramebufferResize(WindowFramebufferResizeEvent const& event);

    private:
        backend::RendererBackend m_backend;
    };
}  // namespace renderer
