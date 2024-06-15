#include <mc/renderer/backend/allocator.hpp>
#include <mc/renderer/backend/gltfloader.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>

#include <filesystem>

#include <fastgltf/core.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace renderer::backend
{
    namespace fs = std::filesystem;

    void RendererBackend::loadGltf(Viewer* viewer, std::filesystem::path path)
    {
        MC_ASSERT_MSG(fs::exists(path), "glTF file path does not exist: {}", path.string());

        logger::info("Loading {}...", path.string());

        {
            static constexpr auto supportedExtensions = fastgltf::Extensions::KHR_mesh_quantization |
                                                        fastgltf::Extensions::KHR_texture_transform |
                                                        fastgltf::Extensions::KHR_materials_variants;

            fastgltf::Parser parser(supportedExtensions);

            constexpr auto gltfOptions = fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers |
                                         fastgltf::Options::LoadExternalBuffers |
                                         fastgltf::Options::LoadExternalImages |
                                         fastgltf::Options::GenerateMeshIndices;

            auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);

            MC_ASSERT_MSG(gltfFile,
                          "Failed to open glTF file {}: {}",
                          path.string(),
                          fastgltf::getErrorMessage(gltfFile.error()));

            auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);

            MC_ASSERT_MSG(asset.error() == fastgltf::Error::None,
                          "Failed to load glTF file {}: {}",
                          path.string(),
                          fastgltf::getErrorMessage(asset.error()));

            viewer->asset = std::move(asset.get());
        }
    }

    void RendererBackend::loadMesh(Viewer* viewer, fastgltf::Mesh& mesh)
    {
        auto& asset = viewer->asset;
        Mesh outMesh {};

        outMesh.primitives.resize(mesh.primitives.size());

        ScopedCommandBuffer cmdBuf(
            m_device, m_commandManager.getTransferCmdPool(), m_device.getTransferQueue());

        for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it)
        {
            auto* positionIt = it->findAttribute("POSITION");

            MC_ASSERT(positionIt != it->attributes.end());  // A mesh primitive is required to hold the
                                                            // POSITION attribute.

            MC_ASSERT(it->indicesAccessor.has_value());  // We specify GenerateMeshIndices,
                                                         // so we should always have indices

            std::size_t baseColorTexcoordIndex = 0;

            // Get the output primitive
            auto index      = std::distance(mesh.primitives.begin(), it);
            auto& primitive = outMesh.primitives[index];

            MC_ASSERT(it->type == fastgltf::PrimitiveType::Triangles);

            if (it->materialIndex.has_value())
            {
                primitive.materialUniformsIndex =
                    it->materialIndex.value() + 1;  // Adjust for default material

                fastgltf::Material& material = viewer->asset.materials[it->materialIndex.value()];

                std::optional<fastgltf::TextureInfo>& baseColorTexture = material.pbrData.baseColorTexture;

                if (baseColorTexture.has_value())
                {
                    fastgltf::Texture& texture = viewer->asset.textures[baseColorTexture->textureIndex];

                    MC_ASSERT(texture.imageIndex.has_value());

                    primitive.albedoTexture = texture.imageIndex.value();

                    if (baseColorTexture->transform && baseColorTexture->transform->texCoordIndex.has_value())
                    {
                        baseColorTexcoordIndex = baseColorTexture->transform->texCoordIndex.value();
                    }
                    else
                    {
                        baseColorTexcoordIndex = material.pbrData.baseColorTexture->texCoordIndex;
                    }
                }
            }
            else
            {
                primitive.materialUniformsIndex = 0;
            }

            GPUBuffer vertexStagingBuffer;
            Vertex* vertices = nullptr;

            {
                // Position
                fastgltf::Accessor& positionAccessor = asset.accessors[positionIt->second];

                // STRICTER
                MC_ASSERT(positionAccessor.bufferViewIndex.has_value());

                // Create the vertex buffer for this primitive, and use the accessor tools
                // to copy directly into the mapped buffer.
                primitive.vertexBuffer = GPUBuffer(m_allocator,
                                                   positionAccessor.count * sizeof(Vertex),
                                                   vk::BufferUsageFlagBits::eTransferDst |
                                                       vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                   VMA_MEMORY_USAGE_AUTO,
                                                   VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

                vertexStagingBuffer = GPUBuffer(m_allocator,
                                                positionAccessor.count * sizeof(Vertex),
                                                vk::BufferUsageFlagBits::eTransferSrc,
                                                VMA_MEMORY_USAGE_AUTO,
                                                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                    VMA_ALLOCATION_CREATE_MAPPED_BIT);

                vertices = static_cast<Vertex*>(vertexStagingBuffer.getMappedData());

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset,
                    positionAccessor,
                    [&](fastgltf::math::fvec3 pos, std::size_t idx)
                    {
                        vertices[idx].position = glm::vec3(pos.x(), pos.y(), pos.z());
                    });
            }

            auto texcoordAttribute = std::string("TEXCOORD_") + std::to_string(baseColorTexcoordIndex);

            if (auto const* texcoord = it->findAttribute(texcoordAttribute); texcoord != it->attributes.end())
            {
                // Tex coord
                auto& texCoordAccessor = asset.accessors[texcoord->second];

                // STRICTER
                MC_ASSERT(texCoordAccessor.bufferViewIndex.has_value());

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                    asset,
                    texCoordAccessor,
                    [&](fastgltf::math::fvec2 uv, std::size_t idx)
                    {
                        vertices[idx].uv_x = uv.x();
                        vertices[idx].uv_y = uv.y();
                    });
            }

            if (auto const* normal = it->findAttribute("NORMAL"); normal != it->attributes.end())
            {
                // Normals
                auto& normalAccessor = asset.accessors[normal->second];

                // STRICTER
                MC_ASSERT(normalAccessor.bufferViewIndex.has_value());

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset,
                    normalAccessor,
                    [&](fastgltf::math::fvec3 normal, std::size_t idx)
                    {
                        vertices[idx].normal = { normal.x(), normal.y(), normal.z() };
                    });
            }

            if (auto const* tangent = it->findAttribute("TANGENT"); tangent != it->attributes.end())
            {
                // Tangents
                auto& tangentAccessor = asset.accessors[tangent->second];

                // STRICTER
                MC_ASSERT(tangentAccessor.bufferViewIndex.has_value());

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
                    asset,
                    tangentAccessor,
                    [&](fastgltf::math::fvec4 tangent, std::size_t idx)
                    {
                        vertices[idx].tangent = { tangent.x(), tangent.y(), tangent.z(), tangent.w() };
                    });
            }

            cmdBuf->copyBuffer(vertexStagingBuffer,
                               primitive.vertexBuffer,
                               vk::BufferCopy().setSize(vertexStagingBuffer.getSize()));

            // Generate the indirect draw command
            vk::DrawIndexedIndirectCommand& draw = primitive.draw;
            draw.instanceCount                   = 1;
            draw.firstInstance                   = 0;
            draw.vertexOffset                    = 0;
            draw.firstIndex                      = 0;

            auto& indexAccessor = asset.accessors[it->indicesAccessor.value()];

            MC_ASSERT(indexAccessor.bufferViewIndex.has_value());

            draw.indexCount = static_cast<std::uint32_t>(indexAccessor.count);

            if (indexAccessor.componentType == fastgltf::ComponentType::UnsignedByte ||
                indexAccessor.componentType == fastgltf::ComponentType::UnsignedShort)
            {
                primitive.indexType = vk::IndexType::eUint16;

                primitive.indexBuffer =
                    GPUBuffer(m_allocator,
                              indexAccessor.count * sizeof(uint16_t),
                              vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                              VMA_MEMORY_USAGE_AUTO,
                              VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

                GPUBuffer indexStagingBuffer(m_allocator,
                                             indexAccessor.count * sizeof(uint16_t),
                                             vk::BufferUsageFlagBits::eTransferSrc,
                                             VMA_MEMORY_USAGE_AUTO,
                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                 VMA_ALLOCATION_CREATE_MAPPED_BIT);
                auto* indices = static_cast<std::uint16_t*>(indexStagingBuffer.getMappedData());

                fastgltf::copyFromAccessor<std::uint16_t>(asset, indexAccessor, indices);

                cmdBuf->copyBuffer(indexStagingBuffer,
                                   primitive.indexBuffer,
                                   vk::BufferCopy().setSize(indexStagingBuffer.getSize()));
            }
            else
            {
                primitive.indexType = vk::IndexType::eUint32;

                primitive.indexBuffer = GPUBuffer(m_allocator,
                                                  indexAccessor.count * sizeof(uint32_t),
                                                  vk::BufferUsageFlagBits::eTransferDst |
                                                      vk::BufferUsageFlagBits::eShaderDeviceAddress,
                                                  VMA_MEMORY_USAGE_AUTO,
                                                  VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

                GPUBuffer indexStagingBuffer(m_allocator,
                                             indexAccessor.count * sizeof(uint32_t),
                                             vk::BufferUsageFlagBits::eTransferSrc,
                                             VMA_MEMORY_USAGE_AUTO,
                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                 VMA_ALLOCATION_CREATE_MAPPED_BIT);

                auto* indices = static_cast<std::uint32_t*>(indexStagingBuffer.getMappedData());

                fastgltf::copyFromAccessor<std::uint32_t>(asset, indexAccessor, indices);

                cmdBuf->copyBuffer(indexStagingBuffer,
                                   primitive.indexBuffer,
                                   vk::BufferCopy().setSize(indexStagingBuffer.getSize()));
            }
        }

        outMesh.drawsBuffer = GPUBuffer();

        outMesh.drawsBuffer =
            GPUBuffer(m_allocator,
                      outMesh.primitives.size() * sizeof(Primitive),
                      vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndirectBuffer,
                      VMA_MEMORY_USAGE_AUTO,
                      VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

        GPUBuffer drawsBufferStaging(m_allocator,
                                     outMesh.drawsBuffer.getSize(),
                                     vk::BufferUsageFlagBits::eTransferSrc,
                                     VMA_MEMORY_USAGE_AUTO,
                                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                         VMA_ALLOCATION_CREATE_MAPPED_BIT);

        std::memcpy(
            drawsBufferStaging.getMappedData(), outMesh.primitives.data(), drawsBufferStaging.getSize());

        cmdBuf->copyBuffer(
            drawsBufferStaging, outMesh.drawsBuffer, vk::BufferCopy().setSize(drawsBufferStaging.getSize()));

        viewer->meshes.emplace_back(std::move(outMesh));
    }

    void RendererBackend::loadImage(Viewer* viewer, fastgltf::Image& image)
    {
        Texture texture;

        std::visit(
            fastgltf::visitor {
                [](auto& arg) {},
                [&](fastgltf::sources::URI& filePath)
                {
                    MC_ASSERT_MSG(filePath.fileByteOffset == 0, "We don't support offsets with stbi");
                    MC_ASSERT_MSG(filePath.uri.isLocalPath(), "We're only capable of loading local files");

                    texture = Texture(m_device, m_allocator, m_commandManager, filePath.uri.path());
                },
                [&](fastgltf::sources::Array& vector)
                {
                    int width, height, nrChannels;
                    unsigned char* data = stbi_load_from_memory(vector.bytes.data(),
                                                                static_cast<int>(vector.bytes.size()),
                                                                &width,
                                                                &height,
                                                                &nrChannels,
                                                                4);

                    texture =
                        Texture(m_device,
                                m_allocator,
                                m_commandManager,
                                vk::Extent2D { static_cast<uint32_t>(width), static_cast<uint32_t>(height) },
                                data,
                                width * height * 4);

                    stbi_image_free(data);
                },
                [&](fastgltf::sources::BufferView& view)
                {
                    auto& bufferView = viewer->asset.bufferViews[view.bufferViewIndex];
                    auto& buffer     = viewer->asset.buffers[bufferView.bufferIndex];
                    // Yes, we've already loaded every buffer into some GL buffer.
                    // However, with GL it's simpler to just copy the buffer data again
                    // for the texture. Besides, this is just an example.
                    std::visit(fastgltf::visitor { // We only care about VectorWithMime here, because we
                                                   // specify LoadExternalBuffers, meaning
                                                   // all buffers are already loaded into a vector.
                                                   [](auto& arg) {},
                                                   [&](fastgltf::sources::Array& vector)
                                                   {
                                                       int width, height, nrChannels;
                                                       unsigned char* data = stbi_load_from_memory(
                                                           vector.bytes.data() + bufferView.byteOffset,
                                                           static_cast<int>(bufferView.byteLength),
                                                           &width,
                                                           &height,
                                                           &nrChannels,
                                                           4);

                                                       texture = Texture(
                                                           m_device,
                                                           m_allocator,
                                                           m_commandManager,
                                                           vk::Extent2D { static_cast<uint32_t>(width),
                                                                          static_cast<uint32_t>(height) },
                                                           data,
                                                           width * height * 4);

                                                       stbi_image_free(data);
                                                   } },
                               buffer.data);
                },
            },
            image.data);

        viewer->textures.emplace_back(std::move(texture));
    }

    void RendererBackend::loadMaterial(Viewer* viewer, fastgltf::Material& material)
    {
        MaterialData materialData = { .baseColorFactor =
                                          glm::make_vec4(material.pbrData.baseColorFactor.data()) };

        if (material.pbrData.baseColorTexture.has_value())
        {
            materialData.flags |= std::to_underlying(MaterialFeatures::ColorTexture);
        }

        // TODO(aether) incomplete

        viewer->materials.push_back(materialData);

        GPUBuffer& buf = viewer->materialBuffers.emplace_back(GPUBuffer(
            m_allocator,
            sizeof(decltype(materialData)),
            vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT));

        std::memcpy(buf.getMappedData(), &materialData, buf.getSize());

        logger::info("Created {} materials", viewer->materialBuffers.size());
    }

    void RendererBackend::prepareScene(Viewer* viewer) {}

    void RendererBackend::drawMesh(vk::CommandBuffer cmdBuf,
                                   Viewer* viewer,
                                   std::size_t meshIndex,
                                   fastgltf::math::fmat4x4 matrix)
    {
        auto& mesh = viewer->meshes[meshIndex];

        vk::Extent2D imageExtent = m_drawImage.getDimensions();

        auto colorAttachment = vk::RenderingAttachmentInfo()
                                   .setImageView(m_drawImage.getImageView())
                                   .setImageLayout(vk::ImageLayout::eGeneral)
                                   .setLoadOp(vk::AttachmentLoadOp::eLoad)
                                   .setStoreOp(vk::AttachmentStoreOp::eStore)
                                   .setResolveImageView(m_drawImageResolve.getImageView())
                                   .setResolveImageLayout(vk::ImageLayout::eGeneral)
                                   .setResolveMode(vk::ResolveModeFlagBits::eAverage);

        auto depthAttachment = vk::RenderingAttachmentInfo()
                                   .setImageView(m_depthImage.getImageView())
                                   .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                   .setLoadOp(vk::AttachmentLoadOp::eClear)
                                   .setStoreOp(vk::AttachmentStoreOp::eStore)
                                   .setClearValue({ .depthStencil = { .depth = 0.f } });

        auto renderInfo = vk::RenderingInfo()
                              .setRenderArea({ .extent = imageExtent })
                              .setColorAttachments(colorAttachment)
                              .setPDepthAttachment(&depthAttachment)
                              .setLayerCount(1);

        cmdBuf.beginRendering(renderInfo);

        vk::Viewport viewport = {
            .x        = 0,
            .y        = 0,
            .width    = static_cast<float>(imageExtent.width),
            .height   = static_cast<float>(imageExtent.height),
            .minDepth = 0.f,
            .maxDepth = 1.f,
        };

        cmdBuf.setViewport(0, viewport);

        auto scissor = vk::Rect2D().setExtent(imageExtent).setOffset({ 0, 0 });

        cmdBuf.setScissor(0, scissor);

        m_stats.drawcall_count = 0;
        m_stats.triangle_count = 0;

        for (auto i = 0u; i < mesh.primitives.size(); ++i)
        {
            Primitive& prim                    = mesh.primitives[i];
            fastgltf::Primitive& gltfPrimitive = viewer->asset.meshes[meshIndex].primitives[i];

            std::size_t materialIndex;

            auto& mappings = gltfPrimitive.mappings;

            if (!mappings.empty() && mappings[viewer->materialVariant].has_value())
            {
                materialIndex = mappings[viewer->materialVariant].value() + 1;  // Adjust for default material
            }
            else
            {
                materialIndex = prim.materialUniformsIndex;
            }

            GPUDrawPushConstants pushConstants {
                .model          = glm::make_mat4(matrix.data()),
                .vertexBuffer   = mesh.primitives[0].vertexBuffer,
                .materialBuffer = materialIndex == 0
                                      ? 0
                                      : m_device->getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(
                                            viewer->materialBuffers[materialIndex - 1])),
                .vertexOffset   = static_cast<uint32_t>(meshIndex)
            };

            // ******
            cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, m_texturedPipeline);

            cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                      m_texturedPipelineLayout,
                                      0,
                                      { m_sceneDataDescriptors },
                                      {});

            cmdBuf.pushConstants(m_texturedPipelineLayout,
                                 vk::ShaderStageFlagBits::eVertex,
                                 0,
                                 sizeof(GPUDrawPushConstants),
                                 &pushConstants);

            cmdBuf.bindIndexBuffer(prim.indexBuffer, 0, prim.indexType);

            cmdBuf.drawIndexedIndirect(mesh.drawsBuffer, i * sizeof(Primitive), 1, sizeof(Primitive));

            m_stats.drawcall_count++;
            m_stats.triangle_count += prim.draw.indexCount / 3;
        }

        cmdBuf.endRendering();
    }

}  // namespace renderer::backend
