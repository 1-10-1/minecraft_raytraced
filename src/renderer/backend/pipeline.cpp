#include <mc/exceptions.hpp>
#include <mc/renderer/backend/pipeline.hpp>
#include <mc/renderer/backend/vertex.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <fstream>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace
{
    auto readShader(std::string_view const& filepath) -> std::vector<char>;
}  // namespace

namespace renderer::backend
{
    Pipeline::Pipeline(Device const& device, RenderPass const& renderPass) : m_device { device }
    {
        auto createShaderModule = [&device](std::string_view const& shaderPath)
        {
            std::vector<char> shaderCode = readShader(shaderPath);

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
            .dynamicStateCount = Utils::size(dynamicStates),
            .pDynamicStates    = dynamicStates.data(),
        };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 0,
            .pVertexBindingDescriptions      = nullptr,  // Optional
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions    = nullptr,  // Optional
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
            .frontFace               = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp          = 0.0f,
            .depthBiasSlopeFactor    = 0.0f,
            .lineWidth               = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo multisampling {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable   = VK_FALSE,
            .minSampleShading      = 1.0f,      // Optional
            .pSampleMask           = nullptr,   // Optional
            .alphaToCoverageEnable = VK_FALSE,  // Optional
            .alphaToOneEnable      = VK_FALSE,  // Optional
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

        VkPipelineLayoutCreateInfo pipelineLayoutInfo {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = 0,
            .pSetLayouts            = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };

        vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_layout) >> vkResultChecker;

        VkGraphicsPipelineCreateInfo pipelineInfo {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = Utils::size(m_shaderStages),
            .pStages             = m_shaderStages.data(),
            .pVertexInputState   = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = nullptr,
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

    Pipeline::~Pipeline()
    {
        vkDestroyPipeline(m_device, m_handle, nullptr);
        vkDestroyPipelineLayout(m_device, m_layout, nullptr);

        vkDestroyShaderModule(m_device, m_fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device, m_vertShaderModule, nullptr);
    }
}  // namespace renderer::backend

namespace
{
    auto readShader(std::string_view const& filepath) -> std::vector<char>
    {
        std::ifstream file(filepath.data(), std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            MC_THROW Error(AssetError, fmt::format("Failed to read shader file at path {}", filepath));
        }

        auto fileSize = file.tellg();
        std::vector<char> buffer(static_cast<std::size_t>(fileSize));

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
}  // namespace
