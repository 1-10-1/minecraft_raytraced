#include <mc/renderer/backend/descriptor.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <ranges>
#include <vulkan/vulkan_core.h>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace renderer::backend
{
    auto DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages) -> VkDescriptorSetLayout
    {
        for (auto& binding : bindings)
        {
            binding.stageFlags |= shaderStages;
        }

        VkDescriptorSetLayoutCreateInfo info = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = utils::size(bindings),
            .pBindings    = bindings.data(),
        };

        VkDescriptorSetLayout set { nullptr };
        vkCreateDescriptorSetLayout(device, &info, nullptr, &set) >> vkResultChecker;

        return set;
    }

    void DescriptorAllocator::initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
    {
        std::vector<VkDescriptorPoolSize> poolSizes(poolRatios.size());

        for (auto [index, ratio] : vi::enumerate(poolRatios))
        {
            poolSizes[index] = { .type = ratio.type, .descriptorCount = static_cast<uint32_t>(ratio.ratio) * maxSets };
        }

        VkDescriptorPoolCreateInfo pool_info = {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags         = 0,
            .maxSets       = maxSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes    = poolSizes.data(),
        };

        vkCreateDescriptorPool(device, &pool_info, nullptr, &m_pool);
    }

    auto DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) -> VkDescriptorSet
    {
        VkDescriptorSetAllocateInfo allocInfo {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = m_pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &layout,
        };

        VkDescriptorSet descriptorSet { VK_NULL_HANDLE };
        vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) >> vkResultChecker;

        return descriptorSet;
    }

    // DescriptorManager::DescriptorManager(Device& device,
    //                                      std::array<UniformBuffer, kNumFramesInFlight> const& uniformBufferArray,
    //                                      VkImageView textureImageView,
    //                                      VkSampler textureSampler)
    //     : m_device { device }
    // {
    //     initLayout();
    //     initPool();
    //     initSets();
    //
    //     for (auto i : vi::iota(0u, kNumFramesInFlight))
    //     {
    //         VkDescriptorBufferInfo bufferInfo {
    //             .buffer = uniformBufferArray[i],
    //             .offset = 0,
    //             .range  = VK_WHOLE_SIZE,
    //         };
    //
    //         VkDescriptorImageInfo imageInfo {
    //             .sampler     = textureSampler,
    //             .imageView   = textureImageView,
    //             .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //         };
    //
    //         // clang-format off
    //         std::array descriptorWrites
    //         {
    //             std::to_array<VkWriteDescriptorSet>({
    //                 {
    //                     .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //                     .dstSet          = m_descriptorSets[i],
    //                     .dstBinding      = 0,
    //                     .dstArrayElement = 0,
    //                     .descriptorCount = 1,
    //                     .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //                     .pBufferInfo     = &bufferInfo,
    //                 },
    //                 {
    //                     .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //                     .dstSet          = m_descriptorSets[i],
    //                     .dstBinding      = 1,
    //                     .dstArrayElement = 0,
    //                     .descriptorCount = 1,
    //                     .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //                     .pImageInfo      = &imageInfo,
    //                 },
    //             })
    //         };
    //         // clang-format on
    //
    //         vkUpdateDescriptorSets(m_device, utils::size(descriptorWrites), descriptorWrites.data(), 0, nullptr);
    //     }
    // }
    //
    // DescriptorManager::~DescriptorManager()
    // {
    //     vkDestroyDescriptorPool(m_device, m_pool, nullptr);
    //     vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
    // }
    //
    // void DescriptorManager::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t currentFrame)
    // {
    //     vkCmdBindDescriptorSets(commandBuffer,
    //                             VK_PIPELINE_BIND_POINT_GRAPHICS,
    //                             pipelineLayout,
    //                             0,
    //                             1,
    //                             &m_descriptorSets[currentFrame],
    //                             0,
    //                             nullptr);
    // }
    //
    // void DescriptorManager::initLayout()
    // {
    //     VkDescriptorSetLayoutBinding uboLayoutBinding {
    //         .binding            = 0,
    //         .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //         .descriptorCount    = 1,
    //         .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
    //         .pImmutableSamplers = nullptr,
    //     };
    //
    //     VkDescriptorSetLayoutBinding samplerLayoutBinding {
    //         .binding            = 1,
    //         .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //         .descriptorCount    = 1,
    //         .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
    //         .pImmutableSamplers = nullptr,
    //     };
    //
    //     std::array bindings { uboLayoutBinding, samplerLayoutBinding };
    //
    //     VkDescriptorSetLayoutCreateInfo layoutInfo {
    //         .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    //         .bindingCount = utils::size(bindings),
    //         .pBindings    = bindings.data(),
    //     };
    //
    //     vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout) >> vkResultChecker;
    // }
    //
    // void DescriptorManager::initPool()
    // {
    //     std::array poolSizes {
    //         VkDescriptorPoolSize {
    //                               .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //                               .descriptorCount = kNumFramesInFlight,
    //                               },
    //
    //         VkDescriptorPoolSize {
    //                               .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //                               .descriptorCount = kNumFramesInFlight * 2, /* x2 because imgui needs one more img sampler */
    //         },
    //     };
    //
    //     VkDescriptorPoolCreateInfo poolInfo {
    //         .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    //         .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    //         .maxSets       = kNumFramesInFlight * 2 /* same thing as above */,
    //         .poolSizeCount = utils::size(poolSizes),
    //         .pPoolSizes    = poolSizes.data(),
    //     };
    //
    //     vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool) >> vkResultChecker;
    // }
    //
    // void DescriptorManager::initSets()
    // {
    //     std::array<VkDescriptorSetLayout, kNumFramesInFlight> layouts {};
    //
    //     layouts.fill(m_layout);
    //
    //     VkDescriptorSetAllocateInfo allocInfo {
    //         .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    //         .descriptorPool     = m_pool,
    //         .descriptorSetCount = kNumFramesInFlight,
    //         .pSetLayouts        = layouts.data(),
    //     };
    //
    //     vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) >> vkResultChecker;
    // }
    //
}  // namespace renderer::backend
