#include <algorithm>
#include <mc/asserts.hpp>
#include <mc/exceptions.hpp>
#include <mc/renderer/backend/pipeline.hpp>
#include <mc/renderer/backend/utils.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <vulkan/vulkan_core.h>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace renderer::backend
{
    auto PipelineLayoutConfig::setPushConstantSettings(uint32_t size, VkShaderStageFlagBits shaderStage)
        -> PipelineLayoutConfig&
    {
        pushConstants = {
            .stageFlags = shaderStage,
            .offset     = 0,
            .size       = size,
        };

        return *this;
    }

    auto PipelineLayoutConfig::setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> const& layout)
        -> PipelineLayoutConfig&
    {
        descriptorSetLayouts = layout;

        return *this;
    }

    auto GraphicsPipelineConfig::addShader(std::filesystem::path const& path,
                                           VkShaderStageFlagBits stage,
                                           std::string const& entryPoint) -> GraphicsPipelineConfig&
    {
        shaders.push_back({ path, stage, entryPoint });

        return *this;
    };

    auto GraphicsPipelineConfig::enableBlending(bool enable) -> GraphicsPipelineConfig&
    {
        blendingEnable = enable;

        return *this;
    }

    auto GraphicsPipelineConfig::blendingSetAlphaBlend() -> GraphicsPipelineConfig&
    {
        srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;

        return *this;
    }

    auto GraphicsPipelineConfig::blendingSetAdditiveBlend() -> GraphicsPipelineConfig&
    {
        srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

        return *this;
    }

    auto GraphicsPipelineConfig::setBlendingWriteMask(VkColorComponentFlagBits mask) -> GraphicsPipelineConfig&
    {
        blendingColorWriteMask = mask;

        return *this;
    }

    auto GraphicsPipelineConfig::setDepthStencilSettings(bool enable,
                                                         VkCompareOp compareOp,
                                                         bool stencilEnable,
                                                         bool enableBoundsTest,
                                                         bool enableWrite) -> GraphicsPipelineConfig&
    {
        depthTestEnable     = enable;
        depthCompareOp      = compareOp;
        depthWriteEnable    = enableWrite;
        depthBoundsTest     = enableBoundsTest;
        this->stencilEnable = stencilEnable;

        return *this;
    };

    auto GraphicsPipelineConfig::setPrimitiveSettings(bool primitiveRestart, VkPrimitiveTopology primitiveTopology)
        -> GraphicsPipelineConfig&
    {
        this->primitiveRestart  = primitiveRestart;
        this->primitiveTopology = primitiveTopology;

        return *this;
    };

    auto GraphicsPipelineConfig::enableRasterizerDiscard(bool enable) -> GraphicsPipelineConfig&
    {
        rasterizerDiscard = enable;

        return *this;
    };

    auto GraphicsPipelineConfig::enableDepthClamp(bool enable) -> GraphicsPipelineConfig&
    {
        depthClampEnabled = enable;

        return *this;
    };

    auto GraphicsPipelineConfig::setLineWidth(float width) -> GraphicsPipelineConfig&
    {
        lineWidth = width;

        return *this;
    };

    auto GraphicsPipelineConfig::setPolygonMode(VkPolygonMode mode) -> GraphicsPipelineConfig&
    {
        polygonMode = mode;

        return *this;
    };

    auto GraphicsPipelineConfig::setCullingSettings(VkCullModeFlags cullMode, VkFrontFace frontFace)
        -> GraphicsPipelineConfig&
    {
        this->cullMode  = cullMode;
        this->frontFace = frontFace;

        return *this;
    };

    auto GraphicsPipelineConfig::setViewportScissorCount(uint32_t viewportCount, uint32_t scissorCount)
        -> GraphicsPipelineConfig&
    {
        this->viewportCount = viewportCount;
        this->scissorCount  = scissorCount;

        return *this;
    };

    auto GraphicsPipelineConfig::setSampleShadingSettings(bool enable, float minSampleShading)
        -> GraphicsPipelineConfig&
    {
        sampleShadingEnable    = enable;
        this->minSampleShading = minSampleShading;

        return *this;
    };

    auto GraphicsPipelineConfig::enableAlphaToOne(bool enable) -> GraphicsPipelineConfig&
    {
        alphaToOneEnable = enable;

        return *this;
    };

    auto GraphicsPipelineConfig::enableAlphaToCoverage(bool enable) -> GraphicsPipelineConfig&
    {
        alphaToCoverageEnable = enable;

        return *this;
    };

    auto GraphicsPipelineConfig::setSampleMask(VkSampleMask mask) -> GraphicsPipelineConfig&
    {
        sampleMask = mask;

        return *this;
    };

    auto GraphicsPipelineConfig::setSampleCount(VkSampleCountFlagBits count) -> GraphicsPipelineConfig&
    {
        rasterizationSamples = count;

        return *this;
    };

    auto GraphicsPipelineConfig::setDepthBiasSettings(bool enable, float constantFactor, float slopeFactor, float clamp)
        -> GraphicsPipelineConfig&
    {
        depthBiasEnabled        = enable;
        depthBiasConstantFactor = constantFactor;
        depthBiasSlopeFactor    = slopeFactor;
        depthBiasClamp          = clamp;

        return *this;
    };

    auto GraphicsPipelineConfig::setColorAttachmentFormat(VkFormat format) -> GraphicsPipelineConfig&
    {
        colorAttachmentFormat = format;

        return *this;
    };

    auto GraphicsPipelineConfig::setDepthAttachmentFormat(VkFormat format) -> GraphicsPipelineConfig&
    {
        depthAttachmentFormat = format;

        return *this;
    };

    GraphicsPipeline::GraphicsPipeline(Device const& device,
                                       PipelineLayout const& layout,
                                       GraphicsPipelineConfig const& config)
        : m_device { &device }
    {
        [[maybe_unused]] auto checkShaderStagePresent = [&shaders = config.shaders](VkShaderStageFlagBits stage)
        {
            return rn::find_if(shaders,
                               [stage](ShaderInfo const& info)
                               {
                                   return info.stage == stage;
                               }) != shaders.end();
        };

        // clang-format off
        MC_ASSERT_MSG(checkShaderStagePresent(VK_SHADER_STAGE_VERTEX_BIT)
                   && checkShaderStagePresent(VK_SHADER_STAGE_FRAGMENT_BIT)
                   && config.shaders.size() >= 2
                   && config.colorAttachmentFormat.has_value()
                   && config.depthAttachmentFormat.has_value(),
                      "Graphics pipeline builder was not correctly configured");
        // clang-format on

        std::array dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicState {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = utils::size(dynamicStates),
            .pDynamicStates    = dynamicStates.data(),
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachment {
            .blendEnable         = static_cast<VkBool32>(config.blendingEnable),
            .srcColorBlendFactor = config.srcColorBlendFactor,
            .dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = config.blendingColorWriteMask,
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
            .depthTestEnable       = static_cast<VkBool32>(config.depthTestEnable),
            .depthWriteEnable      = static_cast<VkBool32>(config.depthWriteEnable),
            .depthCompareOp        = config.depthCompareOp,
            .depthBoundsTestEnable = static_cast<VkBool32>(config.depthBoundsTest),
            .stencilTestEnable     = static_cast<VkBool32>(config.stencilEnable),
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = config.primitiveTopology,
            .primitiveRestartEnable = static_cast<VkBool32>(config.primitiveRestart),
        };

        VkPipelineViewportStateCreateInfo viewportState {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = config.viewportCount,
            .scissorCount  = config.scissorCount,
        };

        VkPipelineRasterizationStateCreateInfo rasterizer {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = static_cast<VkBool32>(config.depthClampEnabled),
            .rasterizerDiscardEnable = static_cast<VkBool32>(config.rasterizerDiscard),
            .polygonMode             = config.polygonMode,
            .cullMode                = config.cullMode,
            .frontFace               = config.frontFace,
            .depthBiasEnable         = static_cast<VkBool32>(config.depthBiasEnabled),
            .depthBiasConstantFactor = config.depthBiasConstantFactor,
            .depthBiasClamp          = config.depthBiasClamp,
            .depthBiasSlopeFactor    = config.depthBiasSlopeFactor,
            .lineWidth               = config.lineWidth,
        };

        VkPipelineMultisampleStateCreateInfo multisampling {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples  = config.rasterizationSamples,
            .sampleShadingEnable   = static_cast<VkBool32>(config.sampleShadingEnable),
            .minSampleShading      = config.minSampleShading,
            .pSampleMask           = config.sampleMask.has_value() ? &config.sampleMask.value() : nullptr,
            .alphaToCoverageEnable = static_cast<VkBool32>(config.alphaToCoverageEnable),
            .alphaToOneEnable      = static_cast<VkBool32>(config.alphaToOneEnable),
        };

        VkPipelineRenderingCreateInfo renderInfo {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount    = 1,
            .pColorAttachmentFormats = &config.colorAttachmentFormat.value(),
            .depthAttachmentFormat   = config.depthAttachmentFormat.value(),
        };

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.reserve(config.shaders.size());

        for (ShaderInfo const& info : config.shaders)
        {
            shaderStages.push_back({ .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                     .stage  = info.stage,
                                     .module = createShaderModule(*m_device, info.path),
                                     .pName  = info.entryPoint.data() });
        }

        VkGraphicsPipelineCreateInfo pipelineInfo {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext               = &renderInfo,
            .stageCount          = utils::size(shaderStages),
            .pStages             = shaderStages.data(),
            .pVertexInputState   = &vertexInput,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = &depthStencil,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = &dynamicState,
            .layout              = layout,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1,
        };

        vkCreateGraphicsPipelines(*m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

        for (auto const& shaderStage : shaderStages)
        {
            vkDestroyShaderModule(*m_device, shaderStage.module, nullptr);
        }
    };

    ComputePipeline::ComputePipeline(Device const& device,
                                     PipelineLayout const& layout,
                                     std::filesystem::path const& path,
                                     std::string_view entryPoint)
        : m_device { &device }
    {
        VkComputePipelineCreateInfo pipelineCreateInfo {
            .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage  = {.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                       .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
                       .module = createShaderModule(*m_device, path),
                       .pName  = entryPoint.data()},
            .layout = layout,
        };

        vkCreateComputePipelines(*m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline) >>
            vkResultChecker;
    }
}  // namespace renderer::backend
