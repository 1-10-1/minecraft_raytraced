#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    RendererBackend::RendererBackend(GLFWwindow* window, glm::uvec2 initialFramebufferDimensions)
        : m_surface { window, m_instance },
          m_device { m_instance, m_surface },
          m_swapchain { m_device, m_surface, initialFramebufferDimensions },
          m_renderPass { m_device, m_surface },
          m_pipeline { m_device, m_renderPass },
          m_framebuffers { m_device, m_renderPass, m_swapchain },
          m_commandManager { m_device }
    {
        createSyncObjects();
    }

    RendererBackend::~RendererBackend()
    {
        if (static_cast<VkDevice>(m_device) != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
        }
        destroySyncObjects();
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

}  // namespace renderer::backend
