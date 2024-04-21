#pragma once

#include "descriptor.hpp"
#include "device.hpp"
#include "render_pass.hpp"

#include <vulkan/vulkan.h>

namespace renderer::backend
{
    class ComputePipeline
    {
    public:
        explicit ComputePipeline(Device const& device);

        ComputePipeline(ComputePipeline const&)                    = delete;
        ComputePipeline(ComputePipeline&&)                         = delete;
        auto operator=(ComputePipeline const&) -> ComputePipeline& = delete;
        auto operator=(ComputePipeline&&) -> ComputePipeline&      = delete;

        ~ComputePipeline();

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkPipeline() const { return m_handle; }

        [[nodiscard]] auto getLayout() const -> VkPipelineLayout { return m_layout; }

    private:
        Device const& m_device;

        VkPipeline m_handle { VK_NULL_HANDLE };

        VkPipelineLayout m_layout {};
        VkShaderModule computeShader {};
    };

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

}  // namespace renderer::backend
