#include <mc/camera.hpp>
#include <mc/events.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/renderer.hpp>

#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <tracy/Tracy.hpp>

namespace renderer
{
    Renderer::Renderer(EventManager& eventManager, window::Window& window, Camera& camera)
        : m_camera { camera },
          m_model { Model("res/models/viking_room.obj") },
          m_backend { backend::RendererBackend(
              window, m_model.meshes[0].getVertices(), m_model.meshes[0].getIndices()) }
    {
        eventManager.subscribe(this, &Renderer::onRender, &Renderer::onUpdate, &Renderer::onFramebufferResize);

        camera.setLens(glm::radians(45.0f), m_backend.getFramebufferSize(), 0.1f, 1000.f);
    }

    void Renderer::onRender(AppRenderEvent const& /* unused */)
    {
        ZoneScopedN("Frontend render");

        m_backend.render();
    }

    void Renderer::onUpdate(AppUpdateEvent const& /* unused */)
    {
        ZoneScopedN("Frontend update");

        m_backend.update(m_camera.getView(), m_camera.getProj());
    }

    void Renderer::onFramebufferResize(WindowFramebufferResizeEvent const& /* event */)
    {
        m_backend.scheduleSwapchainUpdate();
    }
}  // namespace renderer
