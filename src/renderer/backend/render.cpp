#include <mc/logger.hpp>
#include <mc/renderer/backend/render.hpp>
#include <mc/renderer/backend/vk_checker.hpp>

#include <tracy/Tracy.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    void RendererBackend::render()
    {
        ZoneScopedN("Backend render");

        VkCommandBuffer cmdBuf = m_commandManager.getCommandBuffer(m_currentFrame);

        auto frame = m_frameResources[m_currentFrame];

        {
            ZoneNamedN(tracy_fence_wait_zone, "In-flight fence wait", true);
            vkWaitForFences(m_device, 1, &frame.inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
        }

        vkResetFences(m_device, 1, &frame.inFlightFence);

        uint32_t imageIndex {};
        vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, frame.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(cmdBuf, 0);

        {
            ZoneNamedN(tracy_cmd_rec_zone, "Command buffer recording", true);
            recordCommandBuffer(imageIndex);
        }

        std::array waitSemaphores { frame.imageAvailableSemaphore };
        std::array waitStages = std::to_array<VkPipelineStageFlags>({
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        });

        std::array signalSemaphores = { frame.renderFinishedSemaphore };

        VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = waitSemaphores.data(),
            .pWaitDstStageMask  = waitStages.data(),

            .commandBufferCount = 1,
            .pCommandBuffers    = &cmdBuf,

            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = signalSemaphores.data(),
        };

        {
            ZoneNamedN(tracy_queue_submit_zone, "Queue Submit", true);
            vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, frame.inFlightFence) >> vkResultChecker;
        }

        std::array swapChains = { static_cast<VkSwapchainKHR>(m_swapchain) };

        VkPresentInfoKHR presentInfo {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = signalSemaphores.data(),

            .swapchainCount = 1,
            .pSwapchains    = swapChains.data(),
            .pImageIndices  = &imageIndex,

            .pResults = nullptr,
        };

        {
            ZoneNamedN(tracy_queue_present_zone, "Queue present", true);
            vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);
        }

        m_currentFrame = (m_currentFrame + 1) % kNumFramesInFlight;
    }

    void RendererBackend::recordCommandBuffer(uint32_t imageIndex)
    {
        VkCommandBuffer cmdBuf = m_commandManager.getCommandBuffer(m_currentFrame);

        VkExtent2D imageExtent = m_swapchain.getImageExtent();

        VkCommandBufferBeginInfo beginInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags            = 0,
            .pInheritanceInfo = nullptr,
        };

        vkBeginCommandBuffer(cmdBuf, &beginInfo) >> vkResultChecker;

        VkClearValue clearColor = { { { 0.529f, 0.356f, 0.788f, 1.0f } } };

        VkRenderPassBeginInfo renderPassInfo {
            .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass  = m_renderPass,
            .framebuffer = m_framebuffers[imageIndex],
            .renderArea  = {
                .offset = { 0, 0 },
                .extent = imageExtent,
            },
            .clearValueCount = 1,
            .pClearValues    = &clearColor,
        };

        vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        VkViewport viewport {
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = static_cast<float>(imageExtent.width),
            .height   = static_cast<float>(imageExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        VkRect2D scissor {
            .offset = {0, 0},
            .extent = imageExtent,
        };

        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

        vkCmdDraw(cmdBuf, 3, 1, 0, 0);

        vkCmdEndRenderPass(cmdBuf);

        vkEndCommandBuffer(cmdBuf) >> vkResultChecker;
    }
}  // namespace renderer::backend
