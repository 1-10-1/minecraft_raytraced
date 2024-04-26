
#pragma once

#include "mc/renderer/backend/descriptor.hpp"

#include <glm/vec4.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class RendererBackend;

    enum class MaterialPass : uint8_t
    {
        MainColor,
        Transparent,
        Other
    };

    struct MaterialPipeline
    {
        VkPipeline pipeline;
        VkPipelineLayout layout;
    };

    struct MaterialInstance
    {
        MaterialPipeline* pipeline;
        VkDescriptorSet materialSet;
        MaterialPass passType;
    };

    struct GLTFMetallic_Roughness
    {
        MaterialPipeline opaquePipeline {};
        MaterialPipeline transparentPipeline {};

        VkDescriptorSetLayout materialLayout {};

        // NOLINTBEGIN

        struct MaterialConstants
        {
            glm::vec4 colorFactors;
            glm::vec4 metal_rough_factors;
            glm::vec4 extra[14];
        };

        // NOLINTEND

        struct MaterialResources
        {
            VkImageView colorImage;
            VkSampler colorSampler;
            VkImageView metalRoughImage;
            VkSampler metalRoughSampler;
            VkBuffer dataBuffer;
            uint32_t dataBufferOffset;
        };

        DescriptorWriter writer;

        void build_pipelines(RendererBackend* engine);

        void clear_resources(VkDevice device) const { vkDestroyDescriptorSetLayout(device, materialLayout, nullptr); };

        auto write_material(VkDevice device,
                            MaterialPass pass,
                            MaterialResources const& resources,
                            DescriptorAllocatorGrowable& descriptorAllocator) -> MaterialInstance;
    };
}  // namespace renderer::backend

//
