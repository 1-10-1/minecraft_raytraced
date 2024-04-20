#include <glm/ext/matrix_transform.hpp>
#include <mc/asserts.hpp>
#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/constants.hpp>
#include <mc/renderer/backend/image.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/renderer/backend/vk_result_messages.hpp>
#include <mc/timer.hpp>
#include <mc/utils.hpp>

#include <print>

#include <glm/ext.hpp>
#include <glm/mat4x4.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace
{
    using namespace renderer::backend;

    void imguiCheckerFn(VkResult result, std::source_location location = std::source_location::current())
    {
        if (result != VK_SUCCESS)
        {
            MC_THROW Error(GraphicsError, vkResultToStr(result).data(), location);
        }
    }
}  // namespace

namespace renderer::backend
{
    RendererBackend::RendererBackend(window::Window& window,
                                     std::vector<Vertex> const& vertices,
                                     std::vector<uint32_t> const& indices)
    // clang_format off
        : m_surface { window, m_instance },

          m_device { m_instance, m_surface },

          m_commandManager { m_device },

          m_colorAttachmentImage {
                m_device,
                m_commandManager,
                m_surface.getFramebufferExtent(),
                m_surface.getDetails().format,
                m_device.getMaxUsableSampleCount(),
                static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
                                                | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
                VK_IMAGE_ASPECT_COLOR_BIT
            },

          m_depthStencilImage {
                m_device,
                m_commandManager,
                m_surface.getFramebufferExtent(),
                kDepthStencilFormat,
                m_device.getMaxUsableSampleCount(),
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                static_cast<VkImageAspectFlagBits>(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)
            },

          m_swapchain { m_device, m_surface },

          m_renderPass { m_device, m_surface },

          m_framebuffers {
                m_device,
                m_renderPass,
                m_swapchain,
                m_colorAttachmentImage.getImageView(),
                m_depthStencilImage.getImageView()
            },

          m_uniformBuffers {{{ m_device, m_commandManager }, { m_device, m_commandManager }}},

          m_texture{ m_device, m_commandManager, StbiImage("res/textures/viking_room (2).png") },

          m_descriptorManager { m_device, m_uniformBuffers, m_texture.getImageView(), m_texture.getSampler() },

          m_pipeline { m_device, m_renderPass, m_descriptorManager },

          m_vertexBuffer {
                m_device,
                m_commandManager,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                vertices.data(),
                utils::size(vertices) * sizeof(Vertex)
            },

          m_indexBuffer {
                m_device,
                m_commandManager,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                indices.data(),
                utils::size(indices) * sizeof(uint32_t)
            },

          m_numIndices { indices.size() }
    // clang_format on
    {
        initImgui(window.getHandle());

#if PROFILED
        for (size_t i : vi::iota(0u, utils::size(m_frameResources)))
        {
            std::string ctxName = fmt::format("Frame {}/{}", i + 1, kNumFramesInFlight);

            auto& ctx = m_frameResources[i].tracyContext;

            ctx = TracyVkContextCalibrated(
                static_cast<VkPhysicalDevice>(m_device),
                static_cast<VkDevice>(m_device),
                m_device.getGraphicsQueue(),
                m_commandManager.getGraphicsCmdBuffer(i),
                reinterpret_cast<PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT>(
                    vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceCalibratableTimeDomainsEXT")),
                reinterpret_cast<PFN_vkGetCalibratedTimestampsEXT>(
                    vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceCalibratableTimeDomainsEXT")));

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

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        destroySyncObjects();

#if PROFILED
        for (auto& resource : m_frameResources)
        {
            TracyVkDestroy(resource.tracyContext);
        }
#endif
    }

    void RendererBackend::initImgui(GLFWwindow* window)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowRounding = 8.0f;

        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo initInfo {
            .Instance        = m_instance,
            .PhysicalDevice  = m_device,
            .Device          = m_device,
            .QueueFamily     = m_device.getQueueFamilyIndices().graphicsFamily.value(),
            .Queue           = m_device.getGraphicsQueue(),
            .DescriptorPool  = m_descriptorManager.getPool(),
            .RenderPass      = m_renderPass,
            .MinImageCount   = kNumFramesInFlight,
            .ImageCount      = utils::size(m_swapchain.getImageViews()),
            .MSAASamples     = m_device.getMaxUsableSampleCount(),
            .Subpass         = 0,
            .CheckVkResultFn = kDebug ? reinterpret_cast<void (*)(VkResult)>(&imguiCheckerFn) : nullptr,
        };

        ImGui_ImplVulkan_Init(&initInfo);

        [[maybe_unused]] ImFont* font = io.Fonts->AddFontFromFileTTF(
            "./res/fonts/JetBrainsMonoNerdFont-Bold.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesDefault());

        MC_ASSERT(font != nullptr);

        {
            ScopedCommandBuffer cmdBuf {
                m_device, m_commandManager.getGraphicsCmdPool(), m_device.getGraphicsQueue(), true
            };

            ImGui_ImplVulkan_CreateFontsTexture();
        }
    }

    void RendererBackend::update(glm::mat4 view, glm::mat4 projection)
    {
        ZoneScopedN("Backend update");

        updateUniforms(glm::identity<glm::mat4>(), view, projection);
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

        m_surface.refresh(m_device);

        m_framebuffers.destroy();
        m_swapchain.destroy();

        m_colorAttachmentImage.resize(m_surface.getFramebufferExtent());
        m_depthStencilImage.resize(m_surface.getFramebufferExtent());

        m_swapchain.create(m_surface);
        m_framebuffers.create(
            m_renderPass, m_swapchain, m_colorAttachmentImage.getImageView(), m_depthStencilImage.getImageView());
    }

    void RendererBackend::updateUniforms(glm::mat4 model, glm::mat4 view, glm::mat4 projection)
    {
        m_uniformBuffers[m_currentFrame].update({ .model = model, .view = view, .proj = projection });
    }
}  // namespace renderer::backend
