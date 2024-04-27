#include "mc/renderer/backend/descriptor.hpp"
#include <mc/asserts.hpp>
#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/constants.hpp>
#include <mc/renderer/backend/image.hpp>
#include <mc/renderer/backend/info_structs.hpp>
#include <mc/renderer/backend/mesh_loader.hpp>
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
    inline auto generateCheckerboard() -> std::array<uint32_t, static_cast<uint64_t>(16) * 16>
    {
        uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        uint32_t black   = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
        std::array<uint32_t, static_cast<uint64_t>(16) * 16> pixels {};  //for 16x16 checkerboard texture
                                                                         //
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) != 0 ? magenta : black;
            }
        }

        return pixels;
    }

    static auto white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));  // NOLINT

    auto load_image(RendererBackend* engine, fastgltf::Asset& asset, fastgltf::Image& image)
        -> std::optional<std::shared_ptr<Texture>>;

    RendererBackend::RendererBackend(window::Window& window)
        // clang_format off
        : m_surface { window, m_instance },

          m_device { m_instance, m_surface },

          m_swapchain { m_device, m_surface },

          m_allocator { m_instance, m_device },

          m_pipelineManager { m_device },

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

          m_texture { m_device, m_allocator, m_commandManager, { "./res/textures/viking_room (2).png" } },

          m_checkboardTexture { m_device, m_allocator, m_commandManager, VkExtent2D {16, 16}, generateCheckerboard().data(), generateCheckerboard().size() * sizeof(float) },
    
          m_whiteImage { m_device, m_allocator, m_commandManager, VkExtent2D {1, 1}, static_cast<void*>(&white), sizeof(uint32_t) },

          m_materialConstants { m_allocator,
                                sizeof(GLTFMetallic_Roughness::MaterialConstants),
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VMA_MEMORY_USAGE_CPU_TO_GPU }

    // clang_format on
    {
        initImgui(window.getHandle());

        m_gpuSceneDataBuffer = BasicBuffer(
            m_allocator, sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        initDescriptors();

        m_graphicsPipeline = m_pipelineManager.createGraphicsBuilder()
                                 .addShader("shaders/colored_triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main")
                                 .addShader("shaders/colored_triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main")
                                 .setColorAttachmentFormat(m_drawImage.getFormat())
                                 .setDepthAttachmentFormat(kDepthStencilFormat)
                                 .setDepthStencilSettings(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                 .setPushConstantSettings(sizeof(GPUDrawPushConstants), VK_SHADER_STAGE_VERTEX_BIT)
                                 .setCullingSettings(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                 .setSampleCount(m_device.getMaxUsableSampleCount())
                                 .setDescriptorSetLayouts({ m_globalDescriptorLayout })
                                 .build();

        m_metalRoughMaterial.build_pipelines(this);

        GLTFMetallic_Roughness::MaterialResources materialResources { .colorImage        = m_texture.getImageView(),
                                                                      .colorSampler      = m_texture.getSampler(),
                                                                      .metalRoughImage   = m_texture.getImageView(),
                                                                      .metalRoughSampler = m_texture.getSampler() };

        //write the buffer
        auto* sceneUniformData =
            static_cast<GLTFMetallic_Roughness::MaterialConstants*>(m_materialConstants.getMappedData());
        sceneUniformData->colorFactors        = glm::vec4 { 1, 1, 1, 1 };
        sceneUniformData->metal_rough_factors = glm::vec4 { 1, 0.5, 0, 0 };
        materialResources.dataBuffer          = m_materialConstants;
        materialResources.dataBufferOffset    = 0;

        m_defaultData = m_metalRoughMaterial.write_material(
            m_device, MaterialPass::MainColor, materialResources, m_descriptorAllocatorGrowable);

        for (auto& m : m_testMeshes)
        {
            std::shared_ptr<MeshNode> newNode = std::make_shared<MeshNode>();
            newNode->mesh                     = m;

            newNode->localTransform = glm::mat4 { 1.f };
            newNode->worldTransform = glm::mat4 { 1.f };

            for (auto& s : newNode->mesh->surfaces)
            {
                s.material = std::make_shared<GLTFMaterial>(m_defaultData);
            }

            loadedNodes[m->name] = std::move(newNode);
        }

        std::string structurePath = { "res/models/structure.glb" };
        auto structureFile        = loadGltf(this, structurePath);

        assert(structureFile.has_value());

        loadedScenes["structure"] = *structureFile;
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

    void RendererBackend::update_scene() {}

    RendererBackend::~RendererBackend()
    {
        if (static_cast<VkDevice>(m_device) != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
        }

        loadedScenes.clear();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        destroySyncObjects();

        m_descriptorAllocator.destroyPool(m_device);
        vkDestroyDescriptorSetLayout(m_device, m_globalDescriptorLayout, nullptr);
        vkDestroyDescriptorPool(m_device, m_imGuiPool, nullptr);

        m_metalRoughMaterial.clear_resources(m_device);
        m_descriptorAllocatorGrowable.destroy_pools(m_device);

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
            std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          3},
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
            };

            m_descriptorAllocatorGrowable.init(m_device, 10, sizes);
        }
        {
            std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,          3},
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         3},
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
            };

            m_descriptorAllocator.initPool(m_device, 10, sizes);
        }

        {
            m_globalDescriptorLayout = DescriptorLayoutBuilder()
                                           .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                                           .build(m_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        m_globalDescriptorSet = m_descriptorAllocator.allocate(m_device, m_globalDescriptorLayout);

        // Global scene data descriptor set
        {
            DescriptorWriter writer;
            writer.write_buffer(0, m_gpuSceneDataBuffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.update_set(m_device, m_globalDescriptorSet);
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

    void RendererBackend::update(glm::mat4 view, glm::mat4 projection)
    {
        ZoneScopedN("Backend update");

        updateDescriptors(glm::identity<glm::mat4>(), view, projection);
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

    void RendererBackend::updateDescriptors(glm::mat4 model, glm::mat4 view, glm::mat4 projection)
    {
        auto* sceneUniformData = static_cast<GPUSceneData*>(m_gpuSceneDataBuffer.getMappedData());

        *sceneUniformData = GPUSceneData {
            .view              = view,
            .proj              = projection,
            .viewproj          = projection * view,
            .ambientColor      = glm::vec4(.1f),
            .sunlightDirection = glm::vec4(0, 1, 0.5, 1.f),
            .sunPower          = 1.f,
            .sunlightColor     = glm::vec4(1.f),
        };
    }  // namespace renderer::backend

    void MeshNode::Draw(glm::mat4 const& topMatrix, DrawContext& ctx)
    {
        glm::mat4 nodeMatrix = topMatrix * worldTransform;

        for (auto& s : mesh->surfaces)
        {
            RenderObject def {};
            def.indexCount  = s.count;
            def.firstIndex  = s.startIndex;
            def.indexBuffer = mesh->meshBuffers.indexBuffer;
            def.material    = &s.material->data;

            def.transform           = nodeMatrix;
            def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

            ctx.OpaqueSurfaces.push_back(def);
        }

        // recurse down
        Node::Draw(topMatrix, ctx);
    }
}  // namespace renderer::backend
