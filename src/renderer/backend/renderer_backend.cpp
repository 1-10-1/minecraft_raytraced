#include "mc/renderer/backend/descriptor.hpp"
#include "mc/renderer/backend/mesh.hpp"
#include "mc/renderer/backend/pipeline.hpp"
#include <mc/asserts.hpp>
#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/constants.hpp>
#include <mc/renderer/backend/image.hpp>
#include <mc/renderer/backend/info_structs.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/renderer/backend/vk_result_messages.hpp>
#include <mc/timer.hpp>
#include <mc/utils.hpp>

#include <print>

#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>
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

    [[maybe_unused]] void imguiCheckerFn(VkResult result,
                                         std::source_location location = std::source_location::current())
    {
        if (result != VK_SUCCESS)
        {
            MC_THROW Error(GraphicsError, vkResultToStr(result).data(), location);
        }
    }
}  // namespace

namespace renderer::backend
{
    RendererBackend::RendererBackend(window::Window& window)
        // clang_format off
        : m_surface { window, m_instance },

          m_device { m_instance, m_surface },

          m_swapchain { m_device, m_surface },

          m_allocator { m_instance, m_device },

          m_commandManager { m_device },

          m_drawImage { m_device,
                        m_allocator,
                        m_surface.getFramebufferExtent(),
                        VK_FORMAT_R16G16B16A16_SFLOAT,
                        m_device.getMaxUsableSampleCount(),
                        static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                          VK_IMAGE_USAGE_TRANSFER_DST_BIT |  // maybe remove?
                                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
                        VK_IMAGE_ASPECT_COLOR_BIT },

          m_drawImageResolve { m_device,
                               m_allocator,
                               m_drawImage.getDimensions(),
                               m_drawImage.getFormat(),
                               VK_SAMPLE_COUNT_1_BIT,
                               static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT),
                               VK_IMAGE_ASPECT_COLOR_BIT },

          m_depthImage { m_device,
                         m_allocator,
                         m_drawImage.getDimensions(),
                         kDepthStencilFormat,
                         m_device.getMaxUsableSampleCount(),
                         static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),
                         VK_IMAGE_ASPECT_DEPTH_BIT },

          m_diffuseTexture { m_device, m_allocator, m_commandManager, StbiImage("res/textures/container2cube.png") },

          m_specularTexture {
              m_device, m_allocator, m_commandManager, StbiImage("res/textures/container2speccube.png")
          }
    // clang_format on
    {
        initImgui(window.getHandle());

        m_gpuSceneDataBuffer = BasicBuffer(
            m_allocator, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_lightDataBuffer =
            BasicBuffer(m_allocator, sizeof(Light), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_materialDataBuffer =
            BasicBuffer(m_allocator, sizeof(Material), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        initDescriptors();

        auto pipelineLayoutConfig =
            PipelineLayoutConfig()
                .setDescriptorSetLayouts({ m_sceneDataDescriptorLayout })
                .setPushConstantSettings(sizeof(GPUDrawPushConstants), VK_SHADER_STAGE_VERTEX_BIT);

        m_texturelessPipelineLayout = PipelineLayout(m_device, pipelineLayoutConfig);

        pipelineLayoutConfig.setDescriptorSetLayouts({ m_sceneDataDescriptorLayout, m_materialDescriptorLayout });

        m_texturedPipelineLayout = PipelineLayout(m_device, pipelineLayoutConfig);

        {
            auto pipelineConfig = GraphicsPipelineConfig()
                                      .addShader("shaders/main.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main")
                                      .addShader("shaders/main.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main")
                                      .setColorAttachmentFormat(m_drawImage.getFormat())
                                      .setDepthAttachmentFormat(kDepthStencilFormat)
                                      .setDepthStencilSettings(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                      // NOTE: Culling is disabled
                                      .setCullingSettings(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                      .setSampleCount(m_device.getMaxUsableSampleCount())
                                      .setSampleShadingSettings(true, 0.1f);

            m_texturedPipeline = GraphicsPipeline(m_device, m_texturedPipelineLayout, pipelineConfig);
        }

        {
            auto pipelineConfig = GraphicsPipelineConfig()
                                      .addShader("shaders/textureless.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main")
                                      .addShader("shaders/main.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main")
                                      .setColorAttachmentFormat(m_drawImage.getFormat())
                                      .setDepthAttachmentFormat(kDepthStencilFormat)
                                      .setDepthStencilSettings(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                      .setCullingSettings(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                      .setSampleCount(m_device.getMaxUsableSampleCount());

            m_texturelessPipeline = GraphicsPipeline(m_device, m_texturelessPipelineLayout, pipelineConfig);
        }

        {
            Model sphereModel("res/models/uvsphere.obj");
            Model cubeModel("res/models/blendercube.obj");

            auto sphereMesh = uploadMesh(m_device,
                                         m_allocator,
                                         m_commandManager,
                                         sphereModel.meshes[0].getVertices(),
                                         sphereModel.meshes[0].getIndices());

            auto cubeMesh = uploadMesh(m_device,
                                       m_allocator,
                                       m_commandManager,
                                       cubeModel.meshes[0].getVertices(),
                                       cubeModel.meshes[0].getIndices());

            m_light = {
                .position    = {1.5f,                 2.f,             0.f             },
                .color       = { 1.f,                 1.f,             1.f             },
                .attenuation = { .quadratic = 0.017f, .linear = 0.07f, .constant = 1.0f},
            };

            m_renderItems.insert(std::pair {
                "light",
                RenderItem {
                            .model = glm::translate(glm::identity<glm::mat4>(), m_light.position) *
                             glm::scale(glm::identity<glm::mat4>(), { 0.05f, 0.05f, 0.05f }),
                            .meshData = sphereMesh,
                            .layout   = m_texturelessPipelineLayout,
                            .pipeline = m_texturelessPipeline,
                            }
            });

            std::array cubePositions = { glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
                                         glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
                                         glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
                                         glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
                                         glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f) };

            for (uint32_t i = 0; i < 10; i++)
            {
                float angle = 20.0f * static_cast<float>(i);
                auto model  = glm::mat4(1.0f);
                model       = glm::translate(model, cubePositions[i]);
                model       = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
                model       = glm::scale(model, glm::vec3 { 1.f / 3.f, 1.f / 3.f, 1.f / 3.f });

                m_renderItems.insert(std::pair {
                    "model",
                    RenderItem {
                                .model    = model,
                                .meshData = cubeMesh,
                                .layout   = m_texturedPipelineLayout,
                                .pipeline = m_texturedPipeline,
                                }
                });
            }
        }

        m_material = {
            .shininess = 32.f,
        };

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

        m_descriptorAllocator.destroyPool(m_device);
        vkDestroyDescriptorSetLayout(m_device, m_sceneDataDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_materialDescriptorLayout, nullptr);
        vkDestroyDescriptorPool(m_device, m_imGuiPool, nullptr);

#if PROFILED
        for (auto& resource : m_frameResources)
        {
            TracyVkDestroy(resource.tracyContext);
        }
#endif
    }

    void RendererBackend::initDescriptors()
    {
        {
            std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,          4},
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         4},
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
            };

            m_descriptorAllocator.initPool(m_device, 10, sizes);
        }

        {
            m_sceneDataDescriptorLayout =
                DescriptorLayoutBuilder()
                    .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    .build(m_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        {
            m_materialDescriptorLayout = DescriptorLayoutBuilder()
                                             .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                                             .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                                             .addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                                             .build(m_device, VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        m_sceneDataDescriptors = m_descriptorAllocator.allocate(m_device, m_sceneDataDescriptorLayout);
        m_materialDescriptors  = m_descriptorAllocator.allocate(m_device, m_materialDescriptorLayout);

        {
            // Global scene data descriptor set
            DescriptorWriter writer;
            writer.write_buffer(0, m_gpuSceneDataBuffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.write_buffer(1, m_lightDataBuffer, sizeof(Light), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.update_set(m_device, m_sceneDataDescriptors);
        }
        {
            // Texture and material
            DescriptorWriter writer;
            writer.write_image(0,
                               m_diffuseTexture.getImageView(),
                               m_diffuseTexture.getSampler(),
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.write_image(1,
                               m_specularTexture.getImageView(),
                               m_specularTexture.getSampler(),
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.write_buffer(2, m_materialDataBuffer, sizeof(Material), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.update_set(m_device, m_materialDescriptors);
        }
    }

    static auto initImGuiDescriptors(VkDevice device) -> VkDescriptorPool
    {
        std::array poolSizes {
            VkDescriptorPoolSize {
                                  .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  .descriptorCount = kNumFramesInFlight,
                                  },
        };

        VkDescriptorPoolCreateInfo poolInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets       = kNumFramesInFlight,
            .poolSizeCount = utils::size(poolSizes),
            .pPoolSizes    = poolSizes.data(),
        };

        VkDescriptorPool pool = nullptr;
        vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) >> vkResultChecker;

        return pool;
    }

    void RendererBackend::initImgui(GLFWwindow* window)
    {
        m_imGuiPool = initImGuiDescriptors(m_device);

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

        ImGui::SetColorEditOptions(ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB |
                                   ImGuiColorEditFlags_PickerHueBar);

        ImGui_ImplVulkan_InitInfo initInfo {
            .Instance            = m_instance,
            .PhysicalDevice      = m_device,
            .Device              = m_device,
            .QueueFamily         = m_device.getQueueFamilyIndices().graphicsFamily.value(),
            .Queue               = m_device.getGraphicsQueue(),
            .DescriptorPool      = m_imGuiPool,
            .MinImageCount       = kNumFramesInFlight,
            .ImageCount          = utils::size(m_swapchain.getImageViews()),
            .MSAASamples         = VK_SAMPLE_COUNT_1_BIT,
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo =
                VkPipelineRenderingCreateInfo {
                                               .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                                               .colorAttachmentCount    = 1,
                                               .pColorAttachmentFormats = std::array { m_surface.getDetails().format }.data(),
                                               .depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT,
                                               },
            .CheckVkResultFn = kDebug ? reinterpret_cast<void (*)(VkResult)>(&imguiCheckerFn) : nullptr,
        };

        ImGui_ImplVulkan_Init(&initInfo);

        [[maybe_unused]] ImFont* font = io.Fonts->AddFontFromFileTTF(
            "./res/fonts/JetBrainsMonoNerdFont-Bold.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesDefault());

        MC_ASSERT(font != nullptr);

        ImGui_ImplVulkan_CreateFontsTexture();
    }

    void RendererBackend::update(glm::vec3 cameraPos, glm::mat4 view, glm::mat4 projection)
    {
        ZoneScopedN("Backend update");

        // m_timer.tick();
        //
        // float radius = 10.0f;
        //
        // m_lightPos = {
        //     radius *
        //         glm::fastCos(glm::radians(static_cast<float>(m_timer.getTotalTime<Timer::Seconds>().count()) * 90.f)),
        //     radius *
        //         glm::fastCos(glm::radians(static_cast<float>(m_timer.getTotalTime<Timer::Seconds>().count()) * 90.f)),
        //     radius *
        //         glm::fastSin(glm::radians(static_cast<float>(m_timer.getTotalTime<Timer::Seconds>().count()) * 90.f)),
        // };
        //
        // for (auto& [name, item] : m_renderItems)
        // {
        //     [[unlikely]] if (name == "model")
        //     {
        //         continue;
        //     }
        //
        //     item.model = glm::scale(glm::identity<glm::mat4>(), { 0.25f, 0.25f, 0.25f }) *
        //                  glm::translate(glm::identity<glm::mat4>(), m_lightPos);
        // }

        updateDescriptors(cameraPos, glm::identity<glm::mat4>(), view, projection);
    }

    void RendererBackend::createSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo = infoStructs::semaphore_create_info();
        VkFenceCreateInfo fenceInfo         = infoStructs::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

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

    void RendererBackend::handleSurfaceResize()
    {
        vkDeviceWaitIdle(m_device);

        m_surface.refresh(m_device);

        m_swapchain.destroy();
        m_swapchain.create(m_surface);

        m_drawImage.resize(m_allocator, m_surface.getFramebufferExtent());
        m_drawImageResolve.resize(m_allocator, m_surface.getFramebufferExtent());
        m_depthImage.resize(m_allocator, m_surface.getFramebufferExtent());
    }

    void RendererBackend::updateDescriptors(glm::vec3 cameraPos, glm::mat4 model, glm::mat4 view, glm::mat4 projection)
    {
        auto& sceneUniformData    = *static_cast<GPUSceneData*>(m_gpuSceneDataBuffer.getMappedData());
        auto& materialUniformData = *static_cast<Material*>(m_materialDataBuffer.getMappedData());
        auto& lightUniformData    = *static_cast<Light*>(m_lightDataBuffer.getMappedData());

        sceneUniformData = GPUSceneData { .view         = view,
                                          .proj         = projection,
                                          .viewproj     = projection * view,
                                          .ambientColor = glm::vec4(.1f),
                                          .cameraPos    = cameraPos };

        lightUniformData = m_light;

        materialUniformData = m_material;

    }  // namespace renderer::backend
}  // namespace renderer::backend
