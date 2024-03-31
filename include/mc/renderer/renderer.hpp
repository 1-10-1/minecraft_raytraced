#pragma once

#include "../event_manager.hpp"
#include "./backend/renderer_backend.hpp"

#include "glm/fwd.hpp"
#include "mc/events.hpp"

namespace renderer
{
    class Renderer
    {
    public:
        explicit Renderer(EventManager& eventManager, GLFWwindow* window, glm::uvec2 initialFramebufferDimensions);

        void onRender(AppRenderEvent const& /* unused */);
        void onFramebufferResize(WindowFramebufferResizeEvent const& event);

    private:
        backend::RendererBackend m_backend;
    };
}  // namespace renderer
