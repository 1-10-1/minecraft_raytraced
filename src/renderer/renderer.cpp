#include "glm/fwd.hpp"
#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/renderer.hpp>

namespace renderer
{
    Renderer::Renderer(EventManager const& eventManager,
                       GLFWwindow* window,
                       glm::uvec2 initialFramebufferDimensions)
        : m_backend { backend::RendererBackend(window, initialFramebufferDimensions) }
    {
        // This here dont work
        eventManager.addListener(this, &Renderer::onFramebufferResized);
    }

}  // namespace renderer
