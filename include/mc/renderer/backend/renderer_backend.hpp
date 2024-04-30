#pragma once

#include "allocator.hpp"
#include "buffer.hpp"
#include "command.hpp"
#include "constants.hpp"
#include "descriptor.hpp"
#include "device.hpp"
#include "image.hpp"
#include "instance.hpp"
#include "mesh.hpp"
#include "pipeline.hpp"
#include "surface.hpp"
#include "swapchain.hpp"

#include "vk_mem_alloc.h"
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/mat4x4.hpp>
#include <tracy/TracyVulkan.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    struct GPUDrawPushConstants
    {
        glm::mat4 model { glm::identity<glm::mat4>() };
        VkDeviceAddress vertexBuffer {};
    };

    struct alignas(16) GPUSceneData
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewproj;
        glm::vec4 ambientColor;
        glm::vec3 lightPosition;
        float pad1;
        glm::vec3 lightColor;
        float pad2;
        glm::vec3 cameraPos;
    };

    struct alignas(16) Material
    {
        float shininess;
    };

    struct RenderItem
    {
        glm::mat4 model;
        std::shared_ptr<GPUMeshData> meshData;
        VkPipelineLayout layout;
        VkPipeline pipeline;
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
        void update(glm::vec3 cameraPos, glm::mat4 view, glm::mat4 projection);
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

        void toggleLightRevolution() { m_timer.isPaused() ? m_timer.unpause() : m_timer.pause(); }

    private:
        void initImgui(GLFWwindow* window);
        void renderImgui(VkCommandBuffer cmdBuf, VkImageView targetImage);

        void drawGeometry(VkCommandBuffer cmdBuf);

        void initDescriptors();

        void handleSurfaceResize();
        void createSyncObjects();
        void destroySyncObjects();
        void updateDescriptors(glm::vec3 cameraPos, glm::mat4 model, glm::mat4 view, glm::mat4 projection);

        Instance m_instance;
        Surface m_surface;
        Device m_device;
        Swapchain m_swapchain;
        Allocator m_allocator;
        DescriptorAllocator m_descriptorAllocator {};
        CommandManager m_commandManager;

        Image m_drawImage, m_drawImageResolve, m_depthImage;
        VkDescriptorSet m_sceneDataDescriptors {}, m_materialDescriptors {};
        VkDescriptorSetLayout m_sceneDataDescriptorLayout {}, m_materialDescriptorLayout {};

        VkDescriptorPool m_imGuiPool {};

        PipelineLayout m_texturedPipelineLayout, m_texturelessPipelineLayout;
        GraphicsPipeline m_texturedPipeline, m_texturelessPipeline;

        GPUSceneData m_sceneData {};
        BasicBuffer m_gpuSceneDataBuffer, m_materialDataBuffer;

        std::unordered_map<std::string, RenderItem> m_renderItems;

        std::array<FrameResources, kNumFramesInFlight> m_frameResources {};

        Texture m_diffuseTexture, m_specularTexture;
        Material m_material {};

        Timer m_timer;

        glm::vec3 m_lightPos {};
        glm::vec3 m_lightColor {};

        struct EngineStats
        {
            uint64_t triangle_count;
            uint64_t drawcall_count;
        } m_stats {};

        uint32_t m_currentFrame { 0 };

        bool m_windowResized { false };

        uint64_t m_frameCount {};
    };
}  // namespace renderer::backend
