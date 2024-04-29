#include <mc/renderer/backend/image.hpp>
#include <mc/renderer/backend/info_structs.hpp>
#include <mc/renderer/backend/render.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include <glm/glm.hpp>
#include <tracy/Tracy.hpp>
#include <vulkan/vulkan_core.h>

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
                handleSurfaceResize();
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
                handleSurfaceResize();
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

    void RendererBackend::drawGeometry(VkCommandBuffer cmdBuf)
    {
        VkExtent2D imageExtent = m_drawImage.getDimensions();

        VkRenderingAttachmentInfo colorAttachment =
            infoStructs::attachment_info(m_drawImage.getImageView(), nullptr, VK_IMAGE_LAYOUT_GENERAL);

        colorAttachment.resolveImageView   = m_drawImageResolve.getImageView();
        colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_GENERAL;
        colorAttachment.resolveMode        = VK_RESOLVE_MODE_AVERAGE_BIT;

        VkRenderingAttachmentInfo depthAttachment =
            infoStructs::depth_attachment_info(m_depthImage.getImageView(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        VkRenderingInfo renderInfo = infoStructs::rendering_info(imageExtent, &colorAttachment, &depthAttachment);

        vkCmdBeginRendering(cmdBuf, &renderInfo);

        VkViewport viewport = {
            .x        = 0,
            .y        = 0,
            .width    = static_cast<float>(imageExtent.width),
            .height   = static_cast<float>(imageExtent.height),
            .minDepth = 0.f,
            .maxDepth = 1.f,
        };

        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

        // clang-format off
        VkRect2D scissor = {
            .offset = { 0,                          0                            },
            .extent = { .width = imageExtent.width, .height = imageExtent.height }
        };
        // clang-format on

        vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

        m_stats.drawcall_count = 1;
        m_stats.triangle_count = m_meshBuffers.indexCount / 3;

        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_wireframe ? m_wireframePipeline : m_fillPipeline);

        std::array descriptorSets { m_sceneDataDescriptors, m_textureDescriptors };

        vkCmdBindDescriptorSets(cmdBuf,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_graphicsPipelineLayout,
                                0,
                                descriptorSets.size(),
                                descriptorSets.data(),
                                0,
                                nullptr);

        GPUDrawPushConstants push_constants {
            .model        = glm::identity<glm::mat4>(),
            .vertexBuffer = m_meshBuffers.vertexBufferAddress,
        };

        vkCmdPushConstants(cmdBuf,
                           m_graphicsPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,
                           sizeof(GPUDrawPushConstants),
                           &push_constants);

        vkCmdBindIndexBuffer(cmdBuf, m_meshBuffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmdBuf, m_meshBuffers.indexCount, 1, 0, 0, 0);

        vkCmdEndRendering(cmdBuf);
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
            TracyVkZone(tracyCtx, cmdBuf, "Command buffer recording");

            VkImage swapchainImage = m_swapchain.getImages()[imageIndex];
            VkExtent2D imageExtent = m_swapchain.getImageExtent();

            Image::transition(cmdBuf, m_drawImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            Image::transition(
                cmdBuf, m_depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

            VkClearColorValue clearValue {
                {33.f / 255.f, 33.f / 255.f, 33.f / 255.f, 1.f}
            };

            VkImageSubresourceRange range = infoStructs::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

            vkCmdClearColorImage(cmdBuf, m_drawImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);

            Image::transition(
                cmdBuf, m_drawImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            {
                TracyVkZone(tracyCtx, cmdBuf, "Geometry render");

                drawGeometry(cmdBuf);
            }

            {
                TracyVkZone(tracyCtx, cmdBuf, "Draw image copy");

                Image::transition(
                    cmdBuf, m_drawImageResolve, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                Image::transition(
                    cmdBuf, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                m_drawImageResolve.copyTo(cmdBuf, swapchainImage, imageExtent, m_drawImage.getDimensions());
            }

            {
                TracyVkZone(tracyCtx, cmdBuf, "ImGui render");

                renderImgui(cmdBuf, m_swapchain.getImageViews()[imageIndex]);
            }

            Image::transition(
                cmdBuf, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        }

        TracyVkCollect(tracyCtx, cmdBuf);

        vkEndCommandBuffer(cmdBuf) >> vkResultChecker;
    }

    void RendererBackend::renderImgui(VkCommandBuffer cmdBuf, VkImageView targetImage)
    {
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

        VkRenderingAttachmentInfo colorAttachment =
            infoStructs::attachment_info(targetImage, nullptr, VK_IMAGE_LAYOUT_GENERAL);

        VkRenderingInfo renderInfo =
            infoStructs::rendering_info(m_swapchain.getImageExtent(), &colorAttachment, nullptr);

        vkCmdBeginRendering(cmdBuf, &renderInfo);

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

            ImGui::Text("Triangles %i", m_stats.triangle_count);
            ImGui::Text("Draws %i", m_stats.drawcall_count);

            ImGui::End();
        }
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);

        vkCmdEndRendering(cmdBuf);
    }
}  // namespace renderer::backend
