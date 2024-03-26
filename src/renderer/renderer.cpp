#include <mc/logger.hpp>
#include <mc/renderer/renderer.hpp>

namespace renderer
{
    Renderer::Renderer(GLFWwindow* window) : m_backend { backend::RendererBackend(window) } {}

}  // namespace renderer
