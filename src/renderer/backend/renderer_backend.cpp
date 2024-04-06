#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <ranges>

#include <tracy/TracyVulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace renderer::backend
{
    RendererBackend::RendererBackend(window::Window& window)
        : m_surface { window, m_instance },
          m_device { m_instance, m_surface },
          m_swapchain { m_device, m_surface },
          m_renderPass { m_device, m_surface },
          m_framebuffers { m_device, m_renderPass, m_swapchain },
          m_pipeline { m_device, m_renderPass },
          m_commandManager { m_device }
    {
#if PROFILED
        auto vkGetPhysicalDeviceCalibratableTimeDomainsEXT =
            reinterpret_cast<PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT>(
                vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceCalibratableTimeDomainsEXT"));

        auto vkGetCalibratedTimestampsEXT = reinterpret_cast<PFN_vkGetCalibratedTimestampsEXT>(
            vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceCalibratableTimeDomainsEXT"));

        for (size_t i : vi::iota(0u, Utils::size(m_frameResources)))
        {
            std::string ctxName = fmt::format("Frame resource {}", i + 1);

            auto& ctx = m_frameResources[i].tracyContext;

            ctx = TracyVkContextCalibrated(static_cast<VkPhysicalDevice>(m_device),
                                           static_cast<VkDevice>(m_device),
                                           m_device.getGraphicsQueue(),
                                           m_commandManager.getCommandBuffer(i),
                                           vkGetPhysicalDeviceCalibratableTimeDomainsEXT,
                                           vkGetCalibratedTimestampsEXT);

            TracyVkContextName(ctx, ctxName.data(), ctxName.size());
        }
#endif

        createSyncObjects();
    }

    RendererBackend::~RendererBackend()
    {
        if (static_cast<VkDevice>(m_device) != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
        }

        destroySyncObjects();

#if PROFILED
        for (auto& resource : m_frameResources)
        {
            TracyVkDestroy(resource.tracyContext);
        }
#endif
    }

    void RendererBackend::createSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo fenceInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        for (FrameResources& frame : m_frameResources)
        {
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) >> vkResultChecker;
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore) >> vkResultChecker;
            vkCreateFence(m_device, &fenceInfo, nullptr, &frame.inFlightFence) >> vkResultChecker;
        }
    }

    void RendererBackend::destroySyncObjects()
    {
        for (FrameResources& frame : m_frameResources)
        {
            vkDestroySemaphore(m_device, frame.imageAvailableSemaphore, nullptr);
            vkDestroySemaphore(m_device, frame.renderFinishedSemaphore, nullptr);
            vkDestroyFence(m_device, frame.inFlightFence, nullptr);
        }
    }

    void RendererBackend::handleWindowResize()
    {
        m_windowResized = true;
    }

    void RendererBackend::updateSwapchain()
    {
        vkDeviceWaitIdle(m_device);

        m_framebuffers.destroy();
        m_swapchain.destroy();

        m_swapchain.create(m_surface);
        m_framebuffers.create(m_renderPass, m_swapchain);
    }
}  // namespace renderer::backend
