#pragma once

#include "device.hpp"
#include "mc/utils.hpp"
#include "vk_checker.hpp"

#include <filesystem>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    struct ShaderInfo
    {
        std::filesystem::path path;
        VkShaderStageFlagBits stage;
        std::string entryPoint;
    };

    class PipelineLayoutConfig
    {
    public:
        auto setPushConstantSettings(uint32_t size, VkShaderStageFlagBits shaderStage) -> PipelineLayoutConfig&;

        auto setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> const& layout) -> PipelineLayoutConfig&;

    private:
        std::optional<VkPushConstantRange> pushConstants {};
        std::optional<std::vector<VkDescriptorSetLayout>> descriptorSetLayouts {};

        friend class PipelineLayout;
    };

    class GraphicsPipelineConfig
    {
    public:
        auto addShader(std::filesystem::path const& path,
                       VkShaderStageFlagBits stage,
                       std::string const& entryPoint) -> GraphicsPipelineConfig&;

        auto enableBlending(bool enable = true) -> GraphicsPipelineConfig&;
        auto blendingSetAlphaBlend() -> GraphicsPipelineConfig&;
        auto blendingSetAdditiveBlend() -> GraphicsPipelineConfig&;
        auto setBlendingWriteMask(VkColorComponentFlagBits mask) -> GraphicsPipelineConfig&;

        auto setDepthStencilSettings(bool enable,
                                     VkCompareOp compareOp,
                                     bool stencilEnable    = false,
                                     bool enableBoundsTest = false,
                                     bool enableWrite      = true) -> GraphicsPipelineConfig&;

        auto setPrimitiveSettings(bool primitiveRestart,
                                  VkPrimitiveTopology primitiveTopology) -> GraphicsPipelineConfig&;

        auto enableRasterizerDiscard(bool enable = true) -> GraphicsPipelineConfig&;

        auto enableDepthClamp(bool enable = true) -> GraphicsPipelineConfig&;

        auto setLineWidth(float width) -> GraphicsPipelineConfig&;

        auto setPolygonMode(VkPolygonMode mode) -> GraphicsPipelineConfig&;

        auto setCullingSettings(VkCullModeFlags cullMode, VkFrontFace frontFace) -> GraphicsPipelineConfig&;

        auto setViewportScissorCount(uint32_t viewportCount, uint32_t scissorCount) -> GraphicsPipelineConfig&;

        auto setSampleShadingSettings(bool enable, float minSampleShading = 0.2f) -> GraphicsPipelineConfig&;

        auto enableAlphaToOne(bool enable = true) -> GraphicsPipelineConfig&;

        auto enableAlphaToCoverage(bool enable = true) -> GraphicsPipelineConfig&;

        auto setSampleMask(VkSampleMask mask) -> GraphicsPipelineConfig&;

        auto setSampleCount(VkSampleCountFlagBits count) -> GraphicsPipelineConfig&;

        auto setDepthBiasSettings(bool enable          = true,
                                  float constantFactor = 0.f,
                                  float slopeFactor    = 0.f,
                                  float clamp          = 0.f) -> GraphicsPipelineConfig&;

        auto setColorAttachmentFormat(VkFormat format) -> GraphicsPipelineConfig&;

        auto setDepthAttachmentFormat(VkFormat format) -> GraphicsPipelineConfig&;

    private:
        std::vector<ShaderInfo> shaders {};

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
        float minSampleShading                     = 0.3f;
        std::optional<VkSampleMask> sampleMask;

        // blending
        bool blendingEnable = false;
        VkColorComponentFlags blendingColorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Additive blending by default

        // Dynamic rendering
        std::optional<VkFormat> colorAttachmentFormat;
        std::optional<VkFormat> depthAttachmentFormat;

        friend class GraphicsPipeline;
    };

    class PipelineLayout
    {
    public:
        PipelineLayout() = default;

        explicit PipelineLayout(Device const& device, PipelineLayoutConfig const& config) : m_device { &device }
        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount =
                    config.descriptorSetLayouts.has_value() ? utils::size(*config.descriptorSetLayouts) : 0,
                .pSetLayouts = config.descriptorSetLayouts.has_value() ? config.descriptorSetLayouts->data() : nullptr,
                .pushConstantRangeCount = config.pushConstants.has_value() ? 1u : 0,
                .pPushConstantRanges    = config.pushConstants.has_value() ? &config.pushConstants.value() : nullptr,
            };

            vkCreatePipelineLayout(*m_device, &pipelineLayoutInfo, nullptr, &m_layout) >> vkResultChecker;
        }

        ~PipelineLayout()
        {
            if (m_layout && m_device)
            {
                vkDestroyPipelineLayout(*m_device, m_layout, nullptr);
            }
        }

        PipelineLayout(PipelineLayout const&)                    = delete;
        auto operator=(PipelineLayout const&) -> PipelineLayout& = delete;

        PipelineLayout(PipelineLayout&& other) noexcept
        {
            std::swap(m_layout, other.m_layout);
            std::swap(m_device, other.m_device);
        }

        auto operator=(PipelineLayout&& other) noexcept -> PipelineLayout&
        {
            m_layout = VK_NULL_HANDLE;
            m_device = nullptr;
            std::swap(m_layout, other.m_layout);
            std::swap(m_device, other.m_device);

            return *this;
        }

        [[nodiscard]] auto get() const -> VkPipelineLayout { return m_layout; }

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkPipelineLayout() const { return m_layout; }

    private:
        Device const* m_device { nullptr };

        VkPipelineLayout m_layout { VK_NULL_HANDLE };
    };

    class GraphicsPipeline
    {
    public:
        GraphicsPipeline() = default;

        GraphicsPipeline(Device const& device, PipelineLayout const& layout, GraphicsPipelineConfig const& config);

        ~GraphicsPipeline()
        {
            if (m_pipeline && m_device)
            {
                vkDestroyPipeline(*m_device, m_pipeline, nullptr);
            }
        }

        GraphicsPipeline(GraphicsPipeline const& other)              = delete;
        auto operator=(GraphicsPipeline const&) -> GraphicsPipeline& = delete;

        GraphicsPipeline(GraphicsPipeline&& other) noexcept
        {
            std::swap(m_pipeline, other.m_pipeline);
            std::swap(m_device, other.m_device);
        }

        auto operator=(GraphicsPipeline&& other) noexcept -> GraphicsPipeline&
        {
            m_pipeline = VK_NULL_HANDLE;
            m_device   = nullptr;
            std::swap(m_pipeline, other.m_pipeline);
            std::swap(m_device, other.m_device);

            return *this;
        }

        [[nodiscard]] auto get() const -> VkPipeline { return m_pipeline; }

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkPipeline() const { return m_pipeline; }

    private:
        Device const* m_device { nullptr };

        VkPipeline m_pipeline { VK_NULL_HANDLE };
    };

    class ComputePipeline
    {
    public:
        ComputePipeline() = default;

        explicit ComputePipeline(Device const& device,
                                 PipelineLayout const& layout,
                                 std::filesystem::path const& path,
                                 std::string_view entryPoint);

        ComputePipeline(ComputePipeline const&)                    = delete;
        auto operator=(ComputePipeline const&) -> ComputePipeline& = delete;

        ComputePipeline(ComputePipeline&& other) noexcept
        {
            std::swap(m_pipeline, other.m_pipeline);
            std::swap(m_device, other.m_device);
        }

        auto operator=(ComputePipeline&& other) noexcept -> ComputePipeline&
        {
            m_pipeline = VK_NULL_HANDLE;
            m_device   = nullptr;
            std::swap(m_pipeline, other.m_pipeline);
            std::swap(m_device, other.m_device);

            return *this;
        }

        ~ComputePipeline()
        {
            if (m_pipeline && m_device)
            {
                vkDestroyPipeline(*m_device, m_pipeline, nullptr);
            }
        };

        [[nodiscard]] auto setShader(std::filesystem::path const& path,
                                     std::string_view entryPoint) -> ComputePipeline&;

        [[nodiscard]] auto get() const -> VkPipeline { return m_pipeline; }

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkPipeline() const { return m_pipeline; }

    private:
        Device const* m_device { nullptr };

        VkPipeline m_pipeline { VK_NULL_HANDLE };
    };
}  // namespace renderer::backend
