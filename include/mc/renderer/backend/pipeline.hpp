#pragma once

#include "device.hpp"
#include "vertex.hpp"

#include <string_view>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace renderer::backend
{
    struct ComputeEffect
    {
        char const* name {};

        VkPipeline pipeline {};
        VkPipelineLayout layout {};

        ComputePushConstants data;
    };

    class ComputePipeline
    {
    public:
        explicit ComputePipeline(Device const& device);

        ComputePipeline(ComputePipeline const&)                    = delete;
        ComputePipeline(ComputePipeline&&)                         = delete;
        auto operator=(ComputePipeline const&) -> ComputePipeline& = delete;
        auto operator=(ComputePipeline&&) -> ComputePipeline&      = delete;

        ~ComputePipeline();

        [[nodiscard]] auto getLayout() const -> VkPipelineLayout { return m_layout; }

        void build(VkDescriptorSetLayout descriptorSetLayout);

        std::array<ComputeEffect, 2>& getEffects() { return m_effects; }

    private:
        Device const& m_device;

        std::array<ComputeEffect, 2> m_effects {};
        VkPipelineLayout m_layout { VK_NULL_HANDLE };
        std::unordered_map<std::string_view, VkShaderModule> m_shaders {};
    };

#if 0
    class GraphicsPipeline
    {
    public:
        explicit GraphicsPipeline(Device const& device,
                                  RenderPass const& renderPass,
                                  DescriptorManager const& descriptor);

        GraphicsPipeline(GraphicsPipeline const&)                    = delete;
        GraphicsPipeline(GraphicsPipeline&&)                         = delete;
        auto operator=(GraphicsPipeline const&) -> GraphicsPipeline& = delete;
        auto operator=(GraphicsPipeline&&) -> GraphicsPipeline&      = delete;

        ~GraphicsPipeline();

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkPipeline() const { return m_handle; }

        [[nodiscard]] auto getLayout() const -> VkPipelineLayout { return m_layout; }

    private:
        Device const& m_device;

        VkPipeline m_handle { VK_NULL_HANDLE };

        VkPipelineLayout m_layout {};
        VkShaderModule m_vertShaderModule {}, m_fragShaderModule {};
        std::array<VkPipelineShaderStageCreateInfo, 2> m_shaderStages {};
    };
#endif
}  // namespace renderer::backend
