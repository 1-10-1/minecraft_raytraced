#include <vulkan/vulkan.h>

namespace renderer::backend::infoStructs
{
    constexpr auto attachment_info(VkImageView view,
                                   VkClearValue* clear,
                                   VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        -> VkRenderingAttachmentInfo
    {
        return {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext       = nullptr,
            .imageView   = view,
            .imageLayout = layout,
            .loadOp      = clear != nullptr ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = clear != nullptr ? *clear : VkClearValue {},
        };
    }

    constexpr auto depth_attachment_info(VkImageView view, VkImageLayout layout) -> VkRenderingAttachmentInfo
    {
        return { .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                 .imageView   = view,
                 .imageLayout = layout,
                 .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                 .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                 .clearValue { .depthStencil { .depth = 0.f } } };
    }

    constexpr auto rendering_info(VkExtent2D renderExtent,
                                  VkRenderingAttachmentInfo* colorAttachment,
                                  VkRenderingAttachmentInfo* depthAttachment) -> VkRenderingInfo
    {
        return {
            .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext                = nullptr,
            .renderArea           = VkRect2D {VkOffset2D { 0, 0 }, renderExtent},
            .layerCount           = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments    = colorAttachment,
            .pDepthAttachment     = depthAttachment,
            .pStencilAttachment   = nullptr,
        };
    }

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
