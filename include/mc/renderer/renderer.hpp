#pragma once

#include "../event_manager.hpp"
#include "../logger.hpp"
#include "./backend/renderer_backend.hpp"

#include "glm/fwd.hpp"

namespace renderer
{
    class Renderer
    {
    public:
        explicit Renderer(EventManager const& eventManager,
                          GLFWwindow* window,
                          glm::uvec2 initialFramebufferDimensions);

        void render();

        void onFramebufferResized(WindowFramebufferResizeEvent const& event)
        {
            logger::info("Framebuffer resized! {}", x);
        };

    private:
        backend::RendererBackend m_backend;
        int x { 2 };
    };
}  // namespace renderer
