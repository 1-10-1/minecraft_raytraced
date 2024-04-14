#pragma once

#include "descriptor.hpp"
#include "device.hpp"
#include "render_pass.hpp"

#include <vulkan/vulkan.h>

namespace renderer::backend
{
    class Pipeline
    {
    public:
        explicit Pipeline(Device const& device, RenderPass const& renderPass, DescriptorManager const& descriptor);

        Pipeline(Pipeline const&)                    = delete;
        Pipeline(Pipeline&&)                         = delete;
        auto operator=(Pipeline const&) -> Pipeline& = delete;
        auto operator=(Pipeline&&) -> Pipeline&      = delete;

        ~Pipeline();

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
