#include "mc/renderer/backend/pipeline.hpp"
#include <mc/renderer/backend/material.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    void GLTFMetallic_Roughness::build_pipelines(RendererBackend* engine)
    {
        DescriptorLayoutBuilder layoutBuilder;
        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        materialLayout =
            layoutBuilder.build(engine->m_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        std::array layouts = { engine->m_globalDescriptorLayout, materialLayout };

        GraphicsPipelineBuilder builder =
            std::move(engine->m_pipelineManager.createGraphicsBuilder()
                          .setPushConstantSettings(sizeof(GPUDrawPushConstants), VK_SHADER_STAGE_VERTEX_BIT)
                          .setDescriptorSetLayouts({ layouts[0], layouts[1] })  // Careful
                          .addShader("./shaders/main.vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main")
                          .addShader("./shaders/main.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main")
                          .setCullingSettings(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                          .setDepthStencilSettings(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                          .setSampleCount(engine->m_device.getMaxUsableSampleCount())
                          .setColorAttachmentFormat(engine->m_drawImage.getFormat())
                          .setDepthAttachmentFormat(engine->m_depthImage.getFormat()));

        PipelineHandles handles { builder.build() };

        opaquePipeline.pipeline = handles.pipeline;
        opaquePipeline.layout = transparentPipeline.layout = handles.layout;

        (void)builder.enableBlending();
        (void)builder.blendingSetAdditiveBlend();
        (void)builder.setDepthStencilSettings(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

        handles                      = builder.build(transparentPipeline.layout);
        transparentPipeline.pipeline = handles.pipeline;
    }

    auto GLTFMetallic_Roughness::write_material(VkDevice device,
                                                MaterialPass pass,
                                                MaterialResources const& resources,
                                                DescriptorAllocatorGrowable& descriptorAllocator) -> MaterialInstance
    {
        MaterialInstance matData {};
        matData.passType = pass;

        if (pass == MaterialPass::Transparent)
        {
            matData.pipeline = &transparentPipeline;
        }
        else
        {
            matData.pipeline = &opaquePipeline;
        }

        matData.materialSet = descriptorAllocator.allocate(device, materialLayout);

        writer.clear();
        writer.write_buffer(0,
                            resources.dataBuffer,
                            sizeof(MaterialConstants),
                            resources.dataBufferOffset,
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.write_image(1,
                           resources.colorImage,
                           resources.colorSampler,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.write_image(2,
                           resources.metalRoughImage,
                           resources.metalRoughSampler,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        writer.update_set(device, matData.materialSet);

        return matData;
    }

}  // namespace renderer::backend
