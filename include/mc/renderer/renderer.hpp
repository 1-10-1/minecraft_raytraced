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
        explicit Renderer(EventManager& eventManager,
                          GLFWwindow* window,
                          glm::ivec2 initialFramebufferDimensions);

        void render();

    private:
        backend::RendererBackend m_backend;
        int x { 2 };
    };
}  // namespace renderer
