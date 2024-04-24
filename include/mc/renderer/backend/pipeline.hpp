#pragma once

#include "device.hpp"

#include <filesystem>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    struct PipelineHandles
    {
        VkPipeline pipeline { VK_NULL_HANDLE };
        VkPipelineLayout layout { VK_NULL_HANDLE };
        std::vector<VkShaderModule> shaderModules;
    };

    class ComputePipelineBuilder;
    class GraphicsPipelineBuilder;

    // Class responsible for the lifespan of VkPipeline and VkPipelineLayout objects
    class PipelineManager
    {
    public:
        explicit PipelineManager(Device const& device);

        PipelineManager(PipelineManager const&)                    = delete;
        PipelineManager(PipelineManager&&)                         = delete;
        auto operator=(PipelineManager const&) -> PipelineManager& = delete;
        auto operator=(PipelineManager&&) -> PipelineManager&      = delete;

        ~PipelineManager();

        [[nodiscard]] auto createComputeBuilder() -> ComputePipelineBuilder;
        [[nodiscard]] auto createGraphicsBuilder() -> GraphicsPipelineBuilder;

        void removePipeline();  // TODO(aether)

        void pushHandles(PipelineHandles const& handles) { m_handles.push_back(handles); };

    private:
        Device const& m_device;

        std::vector<PipelineHandles> m_handles;
    };

    struct ComputePipelineInfo
    {
        std::optional<VkPushConstantRange> pushConstants {};
        std::optional<VkDescriptorSetLayout> descriptorSetLayout {};
        std::optional<VkPipelineShaderStageCreateInfo> shaderStage {};
    };

    class ComputePipelineBuilder
    {
    public:
        [[nodiscard]] auto build() -> PipelineHandles;

        [[nodiscard]] auto setPushConstantsSize(uint32_t size) -> ComputePipelineBuilder&;
        [[nodiscard]] auto setDescriptorSetLayout(VkDescriptorSetLayout layout) -> ComputePipelineBuilder&;
        [[nodiscard]] auto setShader(std::filesystem::path const& path, std::string_view entryPoint)
            -> ComputePipelineBuilder&;

    private:
        explicit ComputePipelineBuilder(Device const& device, PipelineManager& manager);

        Device const& m_device;
        PipelineManager& m_manager;

        ComputePipelineInfo m_info;

        PipelineHandles m_handles {};

        friend class PipelineManager;
    };

    struct GraphicsPipelineInfo
    {
        std::optional<VkPushConstantRange> pushConstants {};
        std::optional<VkDescriptorSetLayout> descriptorSetLayout {};
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages {};

        // Defaults
        // ********
        // Depth testing
        bool depthTestEnable       = false;
        bool depthWriteEnable      = true;
        bool depthBoundsTest       = false;
        bool stencilEnable         = false;
        VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

        // IA
        bool primitiveRestart                 = false;
        VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Rasterizer
        bool depthClampEnabled    = false;
        bool rasterizerDiscard    = false;
        bool depthBiasEnabled     = false;
        float lineWidth           = 1.f;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode  = VK_CULL_MODE_NONE;
        VkFrontFace frontFace     = VK_FRONT_FACE_CLOCKWISE;

        uint32_t viewportCount { 1 }, scissorCount { 1 };
        float depthBiasConstantFactor { 0.f }, depthBiasClamp { 0.f }, depthBiasSlopeFactor { 0.f };

        // multisampling
        bool sampleShadingEnable   = false;
        bool alphaToCoverageEnable = false;
        bool alphaToOneEnable      = false;

        VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        float minSampleShading                     = 0.1f;
        std::optional<VkSampleMask> sampleMask;

        // blending
        bool blendingEnable = false;
        VkColorComponentFlags blendingColorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Additive blending by default

        // Dynamic rendering
        std::optional<VkFormat> colorAttachmentFormat;
        std::optional<VkFormat> depthAttachmentFormat;
    };

    class GraphicsPipelineBuilder
    {
    public:
        [[nodiscard]] auto build() -> PipelineHandles;

        [[nodiscard]] auto setPushConstantSettings(uint32_t size, VkShaderStageFlagBits shaderStage)
            -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setDescriptorSetLayout(VkDescriptorSetLayout layout) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto addShader(std::filesystem::path const& path,
                                     VkShaderStageFlagBits stage,
                                     std::string_view entryPoint) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto enableBlending(bool enable = true) -> GraphicsPipelineBuilder&;
        [[nodiscard]] auto blendingSetAlphaBlend() -> GraphicsPipelineBuilder&;
        [[nodiscard]] auto blendingSetAdditiveBlend() -> GraphicsPipelineBuilder&;
        [[nodiscard]] auto setBlendingWriteMask(VkColorComponentFlagBits mask) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setDepthStencilSettings(bool enable,
                                                   VkCompareOp compareOp,
                                                   bool stencilEnable    = false,
                                                   bool enableBoundsTest = false,
                                                   bool enableWrite      = true) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setPrimitiveSettings(bool primitiveRestart, VkPrimitiveTopology primitiveTopology)
            -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto enableRasterizerDiscard(bool enable = true) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto enableDepthClamp(bool enable = true) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setLineWidth(float width) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setPolygonMode(VkPolygonMode mode) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setCullingSettings(VkCullModeFlags cullMode, VkFrontFace frontFace)
            -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setViewportScissorCount(uint32_t viewportCount, uint32_t scissorCount)
            -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setSampleShadingSettings(bool enable, float minSampleShading = 0.2f)
            -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto enableAlphaToOne(bool enable = true) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto enableAlphaToCoverage(bool enable = true) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setSampleMask(VkSampleMask mask) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setSampleCount(VkSampleCountFlagBits count) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto
        setDepthBiasSettings(bool enable = true, float constantFactor = 0.f, float slopeFactor = 0.f, float clamp = 0.f)
            -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setColorAttachmentFormat(VkFormat format) -> GraphicsPipelineBuilder&;

        [[nodiscard]] auto setDepthAttachmentFormat(VkFormat format) -> GraphicsPipelineBuilder&;

    private:
        explicit GraphicsPipelineBuilder(Device const& device, PipelineManager& manager);

        Device const& m_device;
        PipelineManager& m_manager;

        GraphicsPipelineInfo m_info;

        PipelineHandles m_handles {};

        friend class PipelineManager;
    };
}  // namespace renderer::backend
