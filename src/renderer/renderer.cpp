#include "mc/events.hpp"
#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/renderer.hpp>

#include <glm/fwd.hpp>
#include <tracy/Tracy.hpp>

namespace renderer
{
    Renderer::Renderer(EventManager& eventManager, window::Window& window)
        : m_backend { backend::RendererBackend(window) }
    {
        eventManager.subscribe(this, &Renderer::onRender, &Renderer::onFramebufferResize);
    }

    void Renderer::onRender(AppRenderEvent const& /* unused */)
    {
        ZoneScopedN("Frontend render");

        m_backend.render();
    }

    void Renderer::onFramebufferResize(WindowFramebufferResizeEvent const& /* event */)
    {
        m_backend.scheduleSwapchainUpdate();
    }
}  // namespace renderer
