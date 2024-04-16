#include <format>
#include <mc/logger.hpp>

#include <mc/renderer/backend/render.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>

#include <tracy/Tracy.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    void RendererBackend::render()
    {
        ZoneScopedN("Backend render");

        [[maybe_unused]] std::string zoneName = std::format("Render frame {}", m_currentFrame + 1);

        ZoneText(zoneName.c_str(), zoneName.size());

        VkCommandBuffer cmdBuf = m_commandManager.getGraphicsCmdBuffer(m_currentFrame);

        auto frame = m_frameResources[m_currentFrame];

        vkWaitForFences(m_device, 1, &frame.inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());

        uint32_t imageIndex {};

        {
            VkResult result = vkAcquireNextImageKHR(
                m_device, m_swapchain, UINT64_MAX, frame.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                updateSwapchain();
                return;
            }

            if (result != VK_SUBOPTIMAL_KHR)
            {
                result >> vkResultChecker;
            }
        }

        vkResetFences(m_device, 1, &frame.inFlightFence);

        vkResetCommandBuffer(cmdBuf, 0);

        recordCommandBuffer(imageIndex);

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
            ZoneNamedN(tracy_queue_present_zone, "Queue presentation", true);
            VkResult result = vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_windowResized)
            {
                updateSwapchain();
                m_windowResized = false;
            }
            else
            {
                result >> vkResultChecker;
            }
        }

        m_currentFrame = (m_currentFrame + 1) % kNumFramesInFlight;
    }

    void RendererBackend::recordCommandBuffer(uint32_t imageIndex)
    {
#if PROFILED
        TracyVkCtx tracyCtx = m_frameResources[m_currentFrame].tracyContext;
#endif

        VkCommandBuffer cmdBuf = m_commandManager.getGraphicsCmdBuffer(m_currentFrame);

        VkExtent2D imageExtent = m_swapchain.getImageExtent();

        VkCommandBufferBeginInfo beginInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags            = 0,
            .pInheritanceInfo = nullptr,
        };

        vkBeginCommandBuffer(cmdBuf, &beginInfo) >> vkResultChecker;

        {
            TracyVkZone(tracyCtx, cmdBuf, "GPU Render");

            std::array clearColors = { VkClearValue { .color = { { 0.529f, 0.356f, 0.788f, 1.0f } } },
                                       VkClearValue { .depthStencil = { 1.0f, 0 } } };

            VkRenderPassBeginInfo renderPassInfo {
                .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass  = m_renderPass,
                .framebuffer = m_framebuffers[imageIndex],
                .renderArea  = {
                    .offset = { 0, 0 },
                    .extent = imageExtent,
                },
                .clearValueCount = utils::size(clearColors),
                .pClearValues    = clearColors.data(),
            };

            vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

            auto vertexBuffers = std::to_array<VkBuffer>({ m_vertexBuffer });
            auto offsets       = std::to_array<VkDeviceSize>({ 0 });

            vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers.data(), offsets.data());

            vkCmdBindIndexBuffer(cmdBuf, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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

            m_descriptorManager.bind(cmdBuf, m_pipeline.getLayout(), m_currentFrame);

            {
                TracyVkNamedZone(tracyCtx, tracy_vkdraw_zone, cmdBuf, "Draw call", true);
                vkCmdDrawIndexed(cmdBuf, m_numIndices, 1, 0, 0, 0);
            }

            vkCmdEndRenderPass(cmdBuf);
        }

        TracyVkCollect(tracyCtx, cmdBuf);

        vkEndCommandBuffer(cmdBuf) >> vkResultChecker;
    }
}  // namespace renderer::backend
