#include <mc/renderer/backend/image.hpp>
#include <mc/renderer/backend/info_structs.hpp>
#include <mc/renderer/backend/render.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include <tracy/Tracy.hpp>
#include <vulkan/vulkan_core.h>

// TODO(aether) implement a deletion queue
namespace renderer::backend
{
    void RendererBackend::render()
    {
        ZoneScopedN("Backend render");

        [[maybe_unused]] std::string zoneName = std::format("Render frame {}", m_currentFrame + 1);

        ZoneText(zoneName.c_str(), zoneName.size());

        auto frame = m_frameResources[m_currentFrame];

        vkWaitForFences(m_device, 1, &frame.inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
        vkResetFences(m_device, 1, &frame.inFlightFence);

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

        VkCommandBuffer cmdBuf = m_commandManager.getGraphicsCmdBuffer(m_currentFrame);

        vkResetCommandBuffer(cmdBuf, 0);

        recordCommandBuffer(imageIndex);

        VkCommandBufferSubmitInfo cmdinfo = infoStructs::command_buffer_submit_info(cmdBuf);

        VkSemaphoreSubmitInfo waitInfo = infoStructs::semaphore_submit_info(
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.imageAvailableSemaphore);

        VkSemaphoreSubmitInfo signalInfo =
            infoStructs::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.renderFinishedSemaphore);

        VkSubmitInfo2 submit = infoStructs::submit_info(&cmdinfo, &signalInfo, &waitInfo);

        {
            ZoneNamedN(tracy_queue_submit_zone, "Queue Submit", true);
            vkQueueSubmit2(m_device.getGraphicsQueue(), 1, &submit, frame.inFlightFence) >> vkResultChecker;
        }

        std::array swapChains = { static_cast<VkSwapchainKHR>(m_swapchain) };

        VkPresentInfoKHR presentInfo {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &frame.renderFinishedSemaphore,

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
        ++m_frameCount;
    }

    void RendererBackend::recordCommandBuffer(uint32_t imageIndex)
    {
#if PROFILED
        TracyVkCtx tracyCtx = m_frameResources[m_currentFrame].tracyContext;
#endif

        VkCommandBuffer cmdBuf = m_commandManager.getGraphicsCmdBuffer(m_currentFrame);

        VkCommandBufferBeginInfo beginInfo =
            infoStructs::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        vkBeginCommandBuffer(cmdBuf, &beginInfo) >> vkResultChecker;

        {
            TracyVkZone(tracyCtx, cmdBuf, "GPU Render");

            // std::array clearColors = { VkClearValue { .color = { { 252.f / 255, 165.f / 255, 144.f / 255, 1.0f } } },
            //                            VkClearValue { .depthStencil = { 1.0f, 0 } } };

            // VkRenderPassBeginInfo renderPassInfo {
            //     .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            //     .renderPass  = m_renderPass,
            //     .framebuffer = m_framebuffers[imageIndex],
            //     .renderArea  = {
            //         .offset = { 0, 0 },
            //         .extent = imageExtent,
            //     },
            //     .clearValueCount = utils::size(clearColors),
            //     .pClearValues    = clearColors.data(),
            // };
            //
            // vkCmdBeginRenderPass(cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            //
            // vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
            //
            // auto vertexBuffers = std::to_array<VkBuffer>({ m_vertexBuffer });
            // auto offsets       = std::to_array<VkDeviceSize>({ 0 });
            //
            // vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers.data(), offsets.data());
            //
            // vkCmdBindIndexBuffer(cmdBuf, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            VkImage swapchainImage          = m_swapchain.getImages()[imageIndex];
            VkExtent2D swapchainImageExtent = m_swapchain.getImageExtent();

            Image::transition(cmdBuf, m_renderImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

            VkClearColorValue clearValue;
            float flash = abs(sin(static_cast<float>(m_frameCount) / 1200.f));
            clearValue  = {
                {0.0f, flash / 2, flash / 2, 1.0f}
            };

            VkImageSubresourceRange clearRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount     = VK_REMAINING_ARRAY_LAYERS,
            };

            //clear image
            vkCmdClearColorImage(cmdBuf, m_renderImage, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

            //make the swapchain image into presentable mode
            Image::transition(cmdBuf, m_renderImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            Image::transition(cmdBuf, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            m_renderImage.copyTo(cmdBuf, swapchainImage, swapchainImageExtent, m_renderImage.getDimensions());

            Image::transition(
                cmdBuf, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            // m_descriptorManager.bind(cmdBuf, m_pipeline.getLayout(), m_currentFrame);

            // {
            //     TracyVkNamedZone(tracyCtx, tracy_vkdraw_zone, cmdBuf, "Draw call", true);
            //     vkCmdDrawIndexed(cmdBuf, m_numIndices, 1, 0, 0, 0);
            // }

            // renderImgui(cmdBuf);

            // vkCmdEndRenderPass(cmdBuf);
        }

        TracyVkCollect(tracyCtx, cmdBuf);

        vkEndCommandBuffer(cmdBuf) >> vkResultChecker;
    }

    void RendererBackend::renderImgui(VkCommandBuffer cmdBuf)
    {
#if PROFILED
        TracyVkCtx tracyCtx = m_frameResources[m_currentFrame].tracyContext;
#endif
        TracyVkNamedZone(tracyCtx, tracy_imguidraw_zone, cmdBuf, "ImGui draw", true);

        ImGuiIO& io = ImGui::GetIO();

        static double frametime                             = 0.0;
        static Timer::Clock::time_point lastFrametimeUpdate = Timer::Clock::now();

        if (auto now = Timer::Clock::now();
            std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(now - lastFrametimeUpdate).count() >
            333.333f)
        {
            frametime           = 1000.f / io.Framerate;
            lastFrametimeUpdate = now;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        window_flags |= ImGuiWindowFlags_NoScrollbar;
        window_flags |= ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoNav;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

        {
            ImGui::Begin("Statistics", nullptr, window_flags);
            ImGui::SetWindowPos({ 0, 0 });
            ImGui::SetWindowSize({});

            ImGui::TextColored(ImVec4(77.5f / 255, 255.f / 255, 125.f / 255, 1.f), "%.2f mspf", frametime);
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 2.5f);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(255.f / 255, 163.f / 255, 77.f / 255, 1.f), "%.0f fps", 1000 / frametime);
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 2.5f);
            ImGui::SameLine();
            ImGui::TextColored(
                ImVec4(255.f / 255, 215.f / 255, 100.f / 255, 1.f), "Vsync: %s", m_surface.getVsync() ? "on" : "off");

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
    }
}  // namespace renderer::backend
