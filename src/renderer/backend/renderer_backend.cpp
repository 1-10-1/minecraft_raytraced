#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/timer.hpp>
#include <mc/utils.hpp>

#include <print>
#include <ranges>

#include <glm/ext.hpp>
#include <glm/mat4x4.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace renderer::backend
{
    RendererBackend::RendererBackend(window::Window& window)
        : m_surface { window, m_instance },
          m_device { m_instance, m_surface },
          m_commandManager { m_device },
          m_swapchain { m_device, m_surface },
          m_renderPass { m_device, m_surface },
          m_framebuffers { m_device, m_renderPass, m_swapchain },
          m_uniformBuffers {{{ m_device, m_commandManager }, { m_device, m_commandManager }}},  // TODO(aether)
          m_descriptorManager { m_device, m_uniformBuffers },
          m_pipeline { m_device, m_renderPass, m_descriptorManager },

          m_vertexBuffer { m_device,
                           m_commandManager,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           vertices.data(),
                           utils::size(vertices) * sizeof(Vertex) },

          m_indexBuffer { m_device,
                          m_commandManager,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          indices.data(),
                          utils::size(indices) * sizeof(decltype(indices)::value_type) }
    {
#if PROFILED
        auto vkGetPhysicalDeviceCalibratableTimeDomainsEXT =
            reinterpret_cast<PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT>(
                vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceCalibratableTimeDomainsEXT"));

        auto vkGetCalibratedTimestampsEXT = reinterpret_cast<PFN_vkGetCalibratedTimestampsEXT>(
            vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceCalibratableTimeDomainsEXT"));

        for (size_t i : vi::iota(0u, utils::size(m_frameResources)))
        {
            std::string ctxName = fmt::format("Frame resource {}", i + 1);

            auto& ctx = m_frameResources[i].tracyContext;

            ctx = TracyVkContextCalibrated(static_cast<VkPhysicalDevice>(m_device),
                                           static_cast<VkDevice>(m_device),
                                           m_device.getGraphicsQueue(),
                                           m_commandManager.getGraphicsCmdBuffer(i),
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

    void RendererBackend::update()
    {
        ZoneScopedN("Backend update");

        updateUniforms();
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

    void RendererBackend::scheduleSwapchainUpdate()
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

    void RendererBackend::updateUniforms()
    {
        static auto startTime = Timer::getCurrentTime<Timer::Seconds>();

        float time = static_cast<float>((startTime - Timer::getCurrentTime<Timer::Seconds>()).count());

        auto framebufExtent = m_surface.getFramebufferExtent();

        UniformBufferObject ubo {
            .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),

            .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),

            .proj =
                glm::perspective(glm::radians(45.0f),
                                 static_cast<float>(framebufExtent.width) / static_cast<float>(framebufExtent.height),
                                 0.1f,
                                 10.0f),

        };

        ubo.proj[1][1] *= -1;

        m_uniformBuffers[m_currentFrame].update(ubo);
    }
}  // namespace renderer::backend
