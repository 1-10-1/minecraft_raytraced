#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>

namespace renderer::backend
{
    RendererBackend::RendererBackend(GLFWwindow* window, glm::uvec2 initialFramebufferDimensions)
        : m_surface { window, m_instance },
          m_device { Device(m_instance, m_surface) },
          m_swapchain { m_device, m_surface, initialFramebufferDimensions }
    {
    }

}  // namespace renderer::backend
