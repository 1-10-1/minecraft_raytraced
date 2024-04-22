#include <mc/exceptions.hpp>
#include <mc/renderer/backend/pipeline.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <vulkan/vulkan_core.h>

namespace
{
    using namespace renderer::backend;

    auto createShaderModule(VkDevice device, std::string_view shaderPath) -> VkShaderModule
    {
        std::vector<char> shaderCode = utils::readBytes(shaderPath);

        VkShaderModuleCreateInfo createInfo {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shaderCode.size(),
            .pCode    = reinterpret_cast<uint32_t const*>(shaderCode.data()),
        };

        VkShaderModule shaderModule {};

        vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) >> vkResultChecker;

        return shaderModule;
    };

}  // namespace

namespace renderer::backend
{
    ComputePipeline::ComputePipeline(Device const& device) : m_device { device } {}

    ComputePipeline::~ComputePipeline()
    {
        for (auto& effect : m_effects)
        {
            vkDestroyPipeline(m_device, effect.pipeline, nullptr);
        }

        vkDestroyPipelineLayout(m_device, m_layout, nullptr);

        for (auto [_, shader] : m_shaders)
        {
            vkDestroyShaderModule(m_device, shader, nullptr);
        }
    }

    void ComputePipeline::build(VkDescriptorSetLayout descriptorSetLayout)
    {
        m_shaders["gradient"] = createShaderModule(m_device, "shaders/gradient_color.comp.spv");
        m_shaders["sky"]      = createShaderModule(m_device, "shaders/sky.comp.spv");

        VkPushConstantRange pushConstants {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset     = 0,
            .size       = sizeof(ComputePushConstants),
        };

        VkPipelineLayoutCreateInfo computeLayout {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext                  = nullptr,
            .setLayoutCount         = 1,
            .pSetLayouts            = &descriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges    = &pushConstants,
        };

        vkCreatePipelineLayout(m_device, &computeLayout, nullptr, &m_layout) >> vkResultChecker;

        VkPipelineShaderStageCreateInfo stageinfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .pName = "main",
        };

        VkComputePipelineCreateInfo computePipelineCreateInfo {
            .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage  = stageinfo,
            .layout = m_layout,
        };

        ComputeEffect gradient {
            .name   = "gradient",
            .layout = m_layout,
            .data   = {.data1 = { 1, 0, 0, 1 }, .data2 = { 0, 0, 1, 1 }},
        };

        ComputeEffect sky {
            .name   = "sky",
            .layout = m_layout,
            .data   = { .data1 = { 0.1, 0.2, 0.4, 0.97 } },
        };

        computePipelineCreateInfo.stage.module = m_shaders["gradient"];
        vkCreateComputePipelines(
            m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline) >>
            vkResultChecker;

        computePipelineCreateInfo.stage.module = m_shaders["sky"];
        vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.pipeline) >>
            vkResultChecker;

        m_effects[0] = gradient;
        m_effects[1] = sky;
    }

    // *******************************************************************************************************************

#if 0
    GraphicsPipeline::GraphicsPipeline(Device const& device,
                                       RenderPass const& renderPass,
                                       DescriptorManager const& descriptor)
        : m_device { device }
    {
        auto createShaderModule = [&device](std::string_view const& shaderPath)
        {
            std::vector<char> shaderCode = utils::readBytes(shaderPath);

            VkShaderModuleCreateInfo createInfo {
                .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = shaderCode.size(),
                .pCode    = reinterpret_cast<uint32_t const*>(shaderCode.data()),
            };

            VkShaderModule shaderModule {};

            vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) >> vkResultChecker;

            return shaderModule;
        };

        // clang-format off
        m_shaderStages = {
            std::to_array<VkPipelineShaderStageCreateInfo>({
                // Vertex Shader
                { .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage  = VK_SHADER_STAGE_VERTEX_BIT,
                  .module = m_vertShaderModule = createShaderModule("shaders/main.vert.spv"),
                  .pName  = "main" },
                    
                // Fragment Shader
                { .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
                  .module = m_fragShaderModule = createShaderModule("shaders/main.frag.spv"),
                  .pName  = "main" },
            })
        };
        // clang-format on

        std::array dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicState {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = utils::size(dynamicStates),
            .pDynamicStates    = dynamicStates.data(),
        };

        VkVertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
        std::array vertexAttributeDescriptions                   = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = &vertexBindingDescription,
            .vertexAttributeDescriptionCount = utils::size(vertexAttributeDescriptions),
            .pVertexAttributeDescriptions    = vertexAttributeDescriptions.data(),
        };

        VkPipelineDepthStencilStateCreateInfo depthStencil {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = VK_TRUE,
            .depthWriteEnable      = VK_TRUE,
            .depthCompareOp        = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable     = VK_FALSE,
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };

        VkPipelineViewportStateCreateInfo viewportState {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount  = 1,
        };

        VkPipelineRasterizationStateCreateInfo rasterizer {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .cullMode                = VK_CULL_MODE_BACK_BIT,
            .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp          = 0.0f,
            .depthBiasSlopeFactor    = 0.0f,
            .lineWidth               = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo multisampling {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples  = m_device.getMaxUsableSampleCount(),
            .sampleShadingEnable   = VK_FALSE,
            .minSampleShading      = 0.1f,
            .pSampleMask           = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable      = VK_FALSE,
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachment {
            .blendEnable         = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo colorBlending {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .logicOp         = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments    = &colorBlendAttachment,
            .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f},
        };

        VkDescriptorSetLayout descriptorLayout = descriptor.getLayout();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = 1,
            .pSetLayouts            = &descriptorLayout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };

        vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_layout) >> vkResultChecker;

        VkGraphicsPipelineCreateInfo pipelineInfo {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = utils::size(m_shaderStages),
            .pStages             = m_shaderStages.data(),
            .pVertexInputState   = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = &depthStencil,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = &dynamicState,
            .layout              = m_layout,
            .renderPass          = renderPass,
            .subpass             = 0,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1,
        };

        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_handle);
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        vkDestroyPipeline(m_device, m_handle, nullptr);
        vkDestroyPipelineLayout(m_device, m_layout, nullptr);

        vkDestroyShaderModule(m_device, m_fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device, m_vertShaderModule, nullptr);
    }
#endif
}  // namespace renderer::backend
