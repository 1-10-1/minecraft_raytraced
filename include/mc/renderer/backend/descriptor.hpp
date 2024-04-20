#pragma once

#include "buffer.hpp"
#include "device.hpp"

#include <array>

namespace renderer::backend
{
    class DescriptorManager
    {
    public:
        explicit DescriptorManager(Device& device,
                                   std::array<UniformBuffer, kNumFramesInFlight> const& uniformBufferArray,
                                   VkImageView textureImageView,
                                   VkSampler textureSampler);
        ~DescriptorManager();

        DescriptorManager(DescriptorManager const&)                    = delete;
        DescriptorManager(DescriptorManager&&)                         = delete;
        auto operator=(DescriptorManager const&) -> DescriptorManager& = delete;
        auto operator=(DescriptorManager&&) -> DescriptorManager&      = delete;

        void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t currentFrame);

        [[nodiscard]] auto getLayout() const -> VkDescriptorSetLayout { return m_layout; }

        [[nodiscard]] auto getPool() const -> VkDescriptorPool { return m_pool; }

    private:
        void initLayout();
        void initPool();
        void initSets();

        Device& m_device;

        VkDescriptorSetLayout m_layout {};
        VkDescriptorPool m_pool {};

        std::array<VkDescriptorSet, kNumFramesInFlight> m_descriptorSets {};
    };
}  // namespace renderer::backend
