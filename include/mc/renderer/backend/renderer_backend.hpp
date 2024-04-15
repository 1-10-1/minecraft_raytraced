#pragma once

#include "command.hpp"
#include "constants.hpp"
#include "device.hpp"
#include "framebuffers.hpp"
#include "instance.hpp"
#include "mc/renderer/backend/buffer.hpp"
#include "pipeline.hpp"
#include "render_pass.hpp"
#include "surface.hpp"
#include "swapchain.hpp"

#include <GLFW/glfw3.h>
#include <glm/ext/vector_uint2.hpp>
#include <tracy/TracyVulkan.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    struct FrameResources
    {
        VkSemaphore imageAvailableSemaphore { VK_NULL_HANDLE };
        VkSemaphore renderFinishedSemaphore { VK_NULL_HANDLE };
        VkFence inFlightFence { VK_NULL_HANDLE };

#if PROFILED
        TracyVkCtx tracyContext {};
#endif
    };

    class RendererBackend
    {
    public:
        explicit RendererBackend(window::Window& window);

        RendererBackend(RendererBackend const&)                    = delete;
        RendererBackend(RendererBackend&&)                         = delete;
        auto operator=(RendererBackend const&) -> RendererBackend& = delete;
        auto operator=(RendererBackend&&) -> RendererBackend&      = delete;

        ~RendererBackend();

        void render();
        void update(glm::mat4 view, glm::mat4 projection);
        void recordCommandBuffer(uint32_t imageIndex);

        void scheduleSwapchainUpdate();

        [[nodiscard]] auto getFramebufferSize() const -> glm::uvec2
        {
            VkExtent2D extent = m_swapchain.getImageExtent();
            return { extent.width, extent.height };
        }

    private:
        void updateSwapchain();
        void createSyncObjects();
        void destroySyncObjects();
        void updateUniforms(glm::mat4 model, glm::mat4 view, glm::mat4 projection);

        Instance m_instance;
        Surface m_surface;
        Device m_device;
        CommandManager m_commandManager;
        Swapchain m_swapchain;
        RenderPass m_renderPass;
        Framebuffers m_framebuffers;

        std::array<UniformBuffer, kNumFramesInFlight> m_uniformBuffers;
        DescriptorManager m_descriptorManager;

        Pipeline m_pipeline;
        StagedBuffer m_vertexBuffer;
        StagedBuffer m_indexBuffer;

        std::array<FrameResources, kNumFramesInFlight> m_frameResources {};

        uint32_t m_currentFrame { 0 };

        bool m_windowResized = false;
    };
}  // namespace renderer::backend
