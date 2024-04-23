#include <mc/asserts.hpp>
#include <mc/exceptions.hpp>
#include <mc/renderer/backend/pipeline.hpp>
#include <mc/renderer/backend/utils.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    PipelineManager::PipelineManager(Device const& device) : m_device { device } {}

    PipelineManager::~PipelineManager()
    {
        for (auto const& handles : m_handles)
        {
            vkDestroyPipeline(m_device, handles.pipeline, nullptr);
            vkDestroyPipelineLayout(m_device, handles.layout, nullptr);

            for (VkShaderModule module : handles.shaderModules)
            {
                vkDestroyShaderModule(m_device, module, nullptr);
            }
        }
    }

    auto PipelineManager::createComputeBuilder() -> ComputePipelineBuilder
    {
        return ComputePipelineBuilder(m_device, *this);
    };

    auto PipelineManager::createGraphicsBuilder() -> GraphicsPipelineBuilder
    {
        return GraphicsPipelineBuilder(m_device, *this);
    };

    ComputePipelineBuilder::ComputePipelineBuilder(Device const& device, PipelineManager& manager)
        : m_device { device }, m_manager { manager }
    {
        m_handles.shaderModules.resize(1);
    }

    auto ComputePipelineBuilder::build() -> PipelineHandles
    {
        // clang-format off
        MC_ASSERT_MSG(m_info.shaderStage.has_value(),
                      "Compute pipeline needs a shader");
        // clang-format on

        VkPipelineLayoutCreateInfo layout {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext          = nullptr,
            .setLayoutCount = m_info.descriptorSetLayout.has_value() ? 1u : 0,
            .pSetLayouts    = m_info.descriptorSetLayout.has_value() ? &m_info.descriptorSetLayout.value() : nullptr,
            .pushConstantRangeCount = m_info.pushConstants.has_value() ? 1u : 0,
            .pPushConstantRanges    = m_info.pushConstants.has_value() ? &m_info.pushConstants.value() : nullptr,
        };

        vkCreatePipelineLayout(m_device, &layout, nullptr, &m_handles.layout) >> vkResultChecker;

        VkComputePipelineCreateInfo pipelineCreateInfo {
            .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage  = m_info.shaderStage.value(),
            .layout = m_handles.layout,
        };

        vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_handles.pipeline) >>
            vkResultChecker;

        m_manager.pushHandles(m_handles);

        return m_handles;
    };

    auto ComputePipelineBuilder::setPushConstantsSize(uint32_t size) -> ComputePipelineBuilder&
    {
        m_info.pushConstants = {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset     = 0,
            .size       = size,
        };

        return *this;
    }

    auto ComputePipelineBuilder::setDescriptorSetLayout(VkDescriptorSetLayout layout) -> ComputePipelineBuilder&
    {
        m_info.descriptorSetLayout = layout;

        return *this;
    }

    auto ComputePipelineBuilder::setShader(std::filesystem::path const& path, std::string_view entryPoint)
        -> ComputePipelineBuilder&
    {
        m_info.shaderStage = {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = createShaderModule(m_device, path),
            .pName  = entryPoint.data(),
        };

        m_handles.shaderModules[0] = m_info.shaderStage->module;

        return *this;
    };

    GraphicsPipelineBuilder::GraphicsPipelineBuilder(Device const& device, PipelineManager& manager)
        : m_device { device }, m_manager { manager }
    {
    }

    auto GraphicsPipelineBuilder::build() -> PipelineHandles
    {
        // clang-format off
        MC_ASSERT_MSG(m_info.shaderStages.size() >= 2
                   && m_info.colorAttachmentFormat.has_value()
                   && m_info.depthAttachmentFormat.has_value(),
                      "Graphics pipeline builder was not correctly configured");
        // clang-format on

        std::array dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicState {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = utils::size(dynamicStates),
            .pDynamicStates    = dynamicStates.data(),
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachment {
            .blendEnable    = static_cast<VkBool32>(m_info.blendingEnable),
            .colorWriteMask = m_info.colorWriteMask,
        };

        VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .logicOp         = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments    = &colorBlendAttachment,
        };

        VkPipelineVertexInputStateCreateInfo vertexInput = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        };

        VkPipelineDepthStencilStateCreateInfo depthStencil {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = static_cast<VkBool32>(m_info.depthTestEnable),
            .depthWriteEnable      = static_cast<VkBool32>(m_info.depthWriteEnable),
            .depthCompareOp        = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = static_cast<VkBool32>(m_info.depthBoundsTest),
            .stencilTestEnable     = static_cast<VkBool32>(m_info.stencilEnable),
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = m_info.primitiveTopology,
            .primitiveRestartEnable = static_cast<VkBool32>(m_info.primitiveRestart),
        };

        VkPipelineViewportStateCreateInfo viewportState {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = m_info.viewportCount,
            .scissorCount  = m_info.scissorCount,
        };

        VkPipelineRasterizationStateCreateInfo rasterizer {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = static_cast<VkBool32>(m_info.depthClampEnabled),
            .rasterizerDiscardEnable = static_cast<VkBool32>(m_info.rasterizerDiscard),
            .polygonMode             = m_info.polygonMode,
            .cullMode                = m_info.cullMode,
            .frontFace               = m_info.frontFace,
            .depthBiasEnable         = static_cast<VkBool32>(m_info.depthBiasEnabled),
            .depthBiasConstantFactor = m_info.depthBiasConstantFactor,
            .depthBiasClamp          = m_info.depthBiasClamp,
            .depthBiasSlopeFactor    = m_info.depthBiasSlopeFactor,
            .lineWidth               = m_info.lineWidth,
        };

        VkPipelineMultisampleStateCreateInfo multisampling {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples  = m_info.rasterizationSamples,
            .sampleShadingEnable   = static_cast<VkBool32>(m_info.sampleShadingEnable),
            .minSampleShading      = m_info.minSampleShading,
            .pSampleMask           = m_info.sampleMask.has_value() ? &m_info.sampleMask.value() : nullptr,
            .alphaToCoverageEnable = static_cast<VkBool32>(m_info.alphaToCoverageEnable),
            .alphaToOneEnable      = static_cast<VkBool32>(m_info.alphaToOneEnable),
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = m_info.descriptorSetLayout.has_value() ? 1u : 0,
            .pSetLayouts    = m_info.descriptorSetLayout.has_value() ? &m_info.descriptorSetLayout.value() : nullptr,
            .pushConstantRangeCount = m_info.pushConstants.has_value() ? 1u : 0,
            .pPushConstantRanges    = m_info.pushConstants.has_value() ? &m_info.pushConstants.value() : nullptr,
        };

        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_handles.layout) >> vkResultChecker;

        VkPipelineRenderingCreateInfo renderInfo {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount    = 1,
            .pColorAttachmentFormats = &m_info.colorAttachmentFormat.value(),
            .depthAttachmentFormat   = m_info.depthAttachmentFormat.value(),
        };

        VkGraphicsPipelineCreateInfo pipelineInfo {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext               = &renderInfo,
            .stageCount          = utils::size(m_info.shaderStages),
            .pStages             = m_info.shaderStages.data(),
            .pVertexInputState   = &vertexInput,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = &depthStencil,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = &dynamicState,
            .layout              = m_handles.layout,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1,
        };

        vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_handles.pipeline);

        m_manager.pushHandles(m_handles);

        return m_handles;
    };

    auto GraphicsPipelineBuilder::setPushConstantsSize(uint32_t size) -> GraphicsPipelineBuilder&
    {
        m_info.pushConstants = {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset     = 0,
            .size       = size,
        };

        return *this;
    }

    auto GraphicsPipelineBuilder::setDescriptorSetLayout(VkDescriptorSetLayout layout) -> GraphicsPipelineBuilder&
    {
        m_info.descriptorSetLayout = layout;

        return *this;
    }

    auto GraphicsPipelineBuilder::addShader(std::filesystem::path const& path,
                                            VkShaderStageFlagBits stage,
                                            std::string_view entryPoint) -> GraphicsPipelineBuilder&
    {
        VkShaderModule module = createShaderModule(m_device, path);

        m_info.shaderStages.push_back({
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = stage,
            .module = module,
            .pName  = entryPoint.data(),
        });

        m_handles.shaderModules.push_back(module);

        return *this;
    };

    auto GraphicsPipelineBuilder::setDepthStencilSettings(bool enable,
                                                          VkCompareOp compareOp,
                                                          bool stencilEnable,
                                                          bool enableBoundsTest,
                                                          bool enableWrite) -> GraphicsPipelineBuilder&
    {
        m_info.depthTestEnable  = enable;
        m_info.depthCompareOp   = compareOp;
        m_info.depthWriteEnable = enableWrite;
        m_info.depthBoundsTest  = enableBoundsTest;
        m_info.stencilEnable    = stencilEnable;

        return *this;
    };

    auto GraphicsPipelineBuilder::setPrimitiveSettings(bool primitiveRestart, VkPrimitiveTopology primitiveTopology)
        -> GraphicsPipelineBuilder&
    {
        m_info.primitiveRestart  = primitiveRestart;
        m_info.primitiveTopology = primitiveTopology;

        return *this;
    };

    auto GraphicsPipelineBuilder::enableRasterizerDiscard(bool enable) -> GraphicsPipelineBuilder&
    {
        m_info.rasterizerDiscard = enable;

        return *this;
    };

    auto GraphicsPipelineBuilder::enableDepthClamp(bool enable) -> GraphicsPipelineBuilder&
    {
        m_info.depthClampEnabled = enable;

        return *this;
    };

    auto GraphicsPipelineBuilder::setLineWidth(float width) -> GraphicsPipelineBuilder&
    {
        m_info.lineWidth = width;

        return *this;
    };

    auto GraphicsPipelineBuilder::setPolygonMode(VkPolygonMode mode) -> GraphicsPipelineBuilder&
    {
        m_info.polygonMode = mode;

        return *this;
    };

    auto GraphicsPipelineBuilder::setCullingSettings(VkCullModeFlags cullMode, VkFrontFace frontFace)
        -> GraphicsPipelineBuilder&
    {
        m_info.cullMode  = cullMode;
        m_info.frontFace = frontFace;

        return *this;
    };

    auto GraphicsPipelineBuilder::setViewportScissorCount(uint32_t viewportCount, uint32_t scissorCount)
        -> GraphicsPipelineBuilder&
    {
        m_info.viewportCount = viewportCount;
        m_info.scissorCount  = scissorCount;

        return *this;
    };

    auto GraphicsPipelineBuilder::setSampleShadingSettings(bool enable, float minSampleShading)
        -> GraphicsPipelineBuilder&
    {
        m_info.sampleShadingEnable = enable;
        m_info.minSampleShading    = minSampleShading;

        return *this;
    };

    auto GraphicsPipelineBuilder::enableAlphaToOne(bool enable) -> GraphicsPipelineBuilder&
    {
        m_info.alphaToOneEnable = enable;

        return *this;
    };

    auto GraphicsPipelineBuilder::enableAlphaToCoverage(bool enable) -> GraphicsPipelineBuilder&
    {
        m_info.alphaToCoverageEnable = enable;

        return *this;
    };

    auto GraphicsPipelineBuilder::setSampleMask(VkSampleMask mask) -> GraphicsPipelineBuilder&
    {
        m_info.sampleMask = mask;

        return *this;
    };

    [[nodiscard]] auto GraphicsPipelineBuilder::setSampleCount(VkSampleCountFlagBits count) -> GraphicsPipelineBuilder&
    {
        m_info.rasterizationSamples = count;

        return *this;
    };

    auto
    GraphicsPipelineBuilder::setDepthBiasSettings(bool enable, float constantFactor, float slopeFactor, float clamp)
        -> GraphicsPipelineBuilder&
    {
        m_info.depthBiasEnabled        = enable;
        m_info.depthBiasConstantFactor = constantFactor;
        m_info.depthBiasSlopeFactor    = slopeFactor;
        m_info.depthBiasClamp          = clamp;

        return *this;
    };

    auto GraphicsPipelineBuilder::setColorAttachmentFormat(VkFormat format) -> GraphicsPipelineBuilder&
    {
        m_info.colorAttachmentFormat = format;

        return *this;
    };

    auto GraphicsPipelineBuilder::setDepthAttachmentFormat(VkFormat format) -> GraphicsPipelineBuilder&
    {
        m_info.depthAttachmentFormat = format;

        return *this;
    };
}  // namespace renderer::backend
