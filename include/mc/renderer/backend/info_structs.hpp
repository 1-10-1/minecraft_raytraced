#include <vulkan/vulkan.h>

namespace renderer::backend::infoStructs
{
    constexpr auto fence_create_info(VkFenceCreateFlags flags = 0) -> VkFenceCreateInfo
    {
        return {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
        };
    }

    constexpr auto semaphore_create_info(VkSemaphoreCreateFlags flags = 0) -> VkSemaphoreCreateInfo
    {
        return {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
        };
    }

    constexpr auto command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0) -> VkCommandBufferBeginInfo
    {
        return {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = flags,
            .pInheritanceInfo = nullptr,
        };
    }

    constexpr auto image_subresource_range(VkImageAspectFlags aspectMask) -> VkImageSubresourceRange
    {
        return {
            .aspectMask     = aspectMask,
            .baseMipLevel   = 0,
            .levelCount     = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount     = VK_REMAINING_ARRAY_LAYERS,
        };
    }

    constexpr auto semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
        -> VkSemaphoreSubmitInfo
    {
        return {
            .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext       = nullptr,
            .semaphore   = semaphore,
            .value       = 1,
            .stageMask   = stageMask,
            .deviceIndex = 0,
        };
    }

    constexpr auto command_buffer_submit_info(VkCommandBuffer cmd) -> VkCommandBufferSubmitInfo
    {
        return {
            .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext         = nullptr,
            .commandBuffer = cmd,
            .deviceMask    = 0,
        };
    }

    constexpr auto submit_info(VkCommandBufferSubmitInfo* cmd,
                               VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                               VkSemaphoreSubmitInfo* waitSemaphoreInfo) -> VkSubmitInfo2
    {
        return {
            .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext                    = nullptr,
            .waitSemaphoreInfoCount   = static_cast<uint32_t>(waitSemaphoreInfo == nullptr ? 0 : 1),
            .pWaitSemaphoreInfos      = waitSemaphoreInfo,
            .commandBufferInfoCount   = 1,
            .pCommandBufferInfos      = cmd,
            .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreInfo == nullptr ? 0 : 1),
            .pSignalSemaphoreInfos    = signalSemaphoreInfo,
        };
    }
}  // namespace renderer::backend::infoStructs
