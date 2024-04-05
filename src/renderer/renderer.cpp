#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/renderer.hpp>

#include <glm/fwd.hpp>
#include <tracy/Tracy.hpp>

namespace renderer
{
    Renderer::Renderer(EventManager& eventManager, GLFWwindow* window, glm::uvec2 initialFramebufferDimensions)
        : m_backend { backend::RendererBackend(window, initialFramebufferDimensions) }
    {
        eventManager.addListener(this, &Renderer::onRender);
        eventManager.addListener(this, &Renderer::onFramebufferResize);
    }

    void Renderer::onRender(AppRenderEvent const& /* unused */)
    {
        ZoneScopedN("Frontend render");
        m_backend.render();
    }

    void Renderer::onFramebufferResize(WindowFramebufferResizeEvent const& event)
    {
        m_backend.recreate_surface(event.dimensions);
    }
}  // namespace renderer
