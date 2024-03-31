#include "glm/fwd.hpp"
#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/renderer.hpp>

namespace renderer
{
    Renderer::Renderer(EventManager& eventManager, GLFWwindow* window, glm::uvec2 initialFramebufferDimensions)
        : m_backend { backend::RendererBackend(eventManager, window, initialFramebufferDimensions) }
    {
    }

}  // namespace renderer
