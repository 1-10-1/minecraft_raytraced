#pragma once

#include "command.hpp"
#include "constants.hpp"
#include "device.hpp"
#include "framebuffers.hpp"
#include "instance.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "surface.hpp"
#include "swapchain.hpp"

#include <GLFW/glfw3.h>
#include <glm/ext/vector_uint2.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    struct FrameResources
    {
        VkSemaphore imageAvailableSemaphore { VK_NULL_HANDLE };
        VkSemaphore renderFinishedSemaphore { VK_NULL_HANDLE };
        VkFence inFlightFence { VK_NULL_HANDLE };
    };

    class RendererBackend
    {
    public:
        explicit RendererBackend(GLFWwindow* window, glm::uvec2 initialFramebufferDimensions);

        RendererBackend(RendererBackend const&)                    = delete;
        RendererBackend(RendererBackend&&)                         = delete;
        auto operator=(RendererBackend const&) -> RendererBackend& = delete;
        auto operator=(RendererBackend&&) -> RendererBackend&      = delete;

        ~RendererBackend();

        void render();
        void recordCommandBuffer(uint32_t imageIndex);

        void recreate_surface(glm::uvec2 dimensions)
        {
            m_surface.refresh(m_device, { .width = dimensions.x, .height = dimensions.y });
        }

    private:
        void createSyncObjects();
        void destroySyncObjects();

        Instance m_instance;
        Surface m_surface;
        Device m_device;
        Swapchain m_swapchain;
        RenderPass m_renderPass;
        Pipeline m_pipeline;
        Framebuffers m_framebuffers;
        CommandManager m_commandManager;

        std::array<FrameResources, kNumFramesInFlight> m_frameResources {};

        uint32_t m_currentFrame { 0 };
    };
}  // namespace renderer::backend
