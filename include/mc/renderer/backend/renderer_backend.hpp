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

// gltf
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <stb_image.h>

namespace renderer::backend
{
    enum class MaterialFeatures : uint32_t
    {
        ColorTexture            = 1 << 0,
        NormalTexture           = 1 << 1,
        RoughnessTexture        = 1 << 2,
        OcclusionTexture        = 1 << 3,
        EmissiveTexture         = 1 << 4,
        TangentVertexAttribute  = 1 << 5,
        TexcoordVertexAttribute = 1 << 6,
    };

    struct alignas(16) MaterialData
    {
        glm::vec4 baseColorFactor;
        glm::mat4 model;
        glm::mat4 modelInv;

        glm::vec3 emissiveFactor;
        float metallicFactor;

        float roughnessFactor;
        float occlusionFactor;
        uint32_t flags;
        uint32_t pad;
    };

    struct MeshDraw
    {
        constexpr static uint16_t invalidBufferIndex = std::numeric_limits<uint16_t>::max();

        uint16_t indexBuffer { invalidBufferIndex };
        uint16_t positionBuffer { invalidBufferIndex };
        uint16_t tangentBuffer { invalidBufferIndex };
        uint16_t normalBuffer { invalidBufferIndex };
        uint16_t texcoordBuffer { invalidBufferIndex };

        BasicBuffer materialBuffer;

        MaterialData materialData;

        // TODO(aether) these are all 0 for now
        uint32_t indexOffset;
        uint32_t positionOffset;
        uint32_t tangentOffset;
        uint32_t normalOffset;
        uint32_t texcoordOffset;

        uint32_t count;

        vk::IndexType indexType;

        vk::DescriptorSet descriptorSet;

        glm::mat4 model;
    };

    struct GltfSceneResources
    {
        std::vector<Texture> textures;
        std::vector<vk::raii::Sampler> samplers;
        std::vector<BasicBuffer> gpuBuffers;
        std::vector<MeshDraw> meshDraws;

        DescriptorAllocatorGrowable descriptorAllocator;
    };

    struct GPUDrawPushConstants
    {
        glm::mat4 model { glm::identity<glm::mat4>() };

        vk::DeviceAddress positionBuffer {};
        vk::DeviceAddress tangentBuffer {};
        vk::DeviceAddress normalBuffer {};
        vk::DeviceAddress texcoordBuffer {};

        uint32_t positionOffset { 0 };
        uint32_t tangentOffset { 0 };
        uint32_t normalOffset { 0 };
        uint32_t texcoordOffset { 0 };
    };

    struct alignas(16) GPUSceneData
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewproj;
        glm::vec4 ambientColor;
        glm::vec3 cameraPos;
        float pad;
        glm::vec3 sunlightDirection;
    };

    struct alignas(16) Material
    {
        glm::vec4 baseColorFactor;
        glm::mat4 model;
        glm::mat4 modelInv;

        glm::vec3 emissiveFactor;
        float metallicFactor;

        float roughnessFactor;
        float occlusionFactor;
        uint32_t flags;
    };

    struct RenderItem
    {
        glm::mat4 model;
        std::shared_ptr<GPUMeshData> meshData;
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };

    struct FrameResources
    {
        vk::raii::Semaphore imageAvailableSemaphore { nullptr };
        vk::raii::Semaphore renderFinishedSemaphore { nullptr };
        vk::raii::Fence inFlightFence { nullptr };

#if PROFILED
        TracyVkCtx tracyContext { nullptr };
#endif
    };

    struct alignas(16) Light
    {
        glm::vec3 position;
        float pad1;

        glm::vec3 color;
        float pad2;

        struct AttenuationFactors
        {
            float quadratic, linear, constant;
        } attenuation;
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
            vk::Extent2D extent = m_swapchain.getImageExtent();
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
        void renderImgui(vk::CommandBuffer cmdBuf, vk::ImageView targetImage);

        void drawGeometry(vk::CommandBuffer cmdBuf);

        void initDescriptors();

        void processGltf();
        auto loadImage(fastgltf::Asset& asset,
                       fastgltf::Image& image,
                       std::filesystem::path gltfDir) -> std::optional<Texture>;
        auto loadBuffer(fastgltf::Asset& asset,
                        fastgltf::Buffer& buffer) -> std::optional<std::vector<uint8_t>>;

        void handleSurfaceResize();
        void createSyncObjects();
        void destroySyncObjects();
        void updateDescriptors(glm::vec3 cameraPos, glm::mat4 model, glm::mat4 view, glm::mat4 projection);

        Instance m_instance;
        Surface m_surface;
        Device m_device;
        Swapchain m_swapchain;
        Allocator m_allocator;
        DescriptorAllocator m_descriptorAllocator;
        CommandManager m_commandManager;

        Image m_drawImage, m_drawImageResolve, m_depthImage;
        vk::DescriptorSet m_sceneDataDescriptors { nullptr };
        vk::raii::DescriptorSetLayout m_sceneDataDescriptorLayout { nullptr },
            m_materialDescriptorLayout { nullptr };

        vk::raii::DescriptorPool m_imGuiPool { nullptr };

        PipelineLayout m_texturedPipelineLayout, m_texturelessPipelineLayout;
        GraphicsPipeline m_texturedPipeline, m_texturelessPipeline;

        GPUSceneData m_sceneData {};
        BasicBuffer m_gpuSceneDataBuffer, m_lightDataBuffer;

        std::unordered_multimap<std::string, RenderItem> m_renderItems;

        std::array<FrameResources, kNumFramesInFlight> m_frameResources {};

        vk::raii::Sampler m_dummySampler { nullptr };
        Texture m_dummyTexture {};

        Timer m_timer;

        Light m_light {};

        GltfSceneResources m_gltfResources;

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
