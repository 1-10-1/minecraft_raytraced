#pragma once

#include "allocator.hpp"
#include "command.hpp"
#include "constants.hpp"
#include "descriptor.hpp"
#include "device.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "mesh_loader.hpp"
#include "pipeline.hpp"
#include "surface.hpp"
#include "swapchain.hpp"
#include "vertex.hpp"

#include "vk_mem_alloc.h"
#include <GLFW/glfw3.h>
#include <glm/ext/vector_uint2.hpp>
#include <glm/mat4x4.hpp>
#include <tracy/TracyVulkan.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    struct ComputeEffect
    {
        std::string_view name {};
        PipelineHandles handles {};
        ComputePushConstants data;
    };

    struct GPUDrawPushConstants
    {
        glm::mat4 worldMatrix;
        VkDeviceAddress vertexBuffer;
    };

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

        void toggleVsync()
        {
            m_surface.scheduleVsyncChange(!m_surface.getVsync());
            scheduleSwapchainUpdate();
        }

    private:
        void initImgui(GLFWwindow* window);
        void renderImgui(VkCommandBuffer cmdBuf, VkImageView targetImage);

        void drawSky(VkCommandBuffer cmdBuf, VkExtent2D imageExtent);
        void drawGeometry(VkCommandBuffer cmdBuf);

        void initDescriptors();

        void updateSwapchain();
        void createSyncObjects();
        void destroySyncObjects();
        void updateUniforms(glm::mat4 model, glm::mat4 view, glm::mat4 projection);

        Instance m_instance;
        Surface m_surface;
        Device m_device;
        Allocator m_allocator;
        DescriptorAllocator m_descriptorAllocator {};
        PipelineManager m_pipelineManager;
        CommandManager m_commandManager;

        Image m_drawImage, m_depthImage;
        VkDescriptorSet m_drawImageDescriptors { VK_NULL_HANDLE };
        VkDescriptorSetLayout m_drawImageDescriptorLayout { VK_NULL_HANDLE };
        VkDescriptorPool m_imGuiPool { VK_NULL_HANDLE };

        PipelineHandles m_graphicsPipeline;

        ComputeEffect m_skyEffect;

        std::vector<std::shared_ptr<MeshAsset>> m_testMeshes;

        glm::mat4 m_mvp;

        Swapchain m_swapchain;

        std::array<FrameResources, kNumFramesInFlight> m_frameResources {};

        uint32_t m_currentFrame { 0 };

        bool m_windowResized { false };

        uint64_t m_frameCount {};
    };
}  // namespace renderer::backend
