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
#include <mc/renderer/backend/utils.hpp>
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
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_structs.hpp>

#include <filesystem>
#include <ranges>

namespace
{
    using namespace renderer::backend;

    [[maybe_unused]] void imguiCheckerFn(vk::Result result,
                                         std::source_location location = std::source_location::current())
    {
        MC_ASSERT_LOC(result == vk::Result::eSuccess, location);
    }
}  // namespace

namespace renderer::backend
{

    namespace fs = std::filesystem;
    namespace fg = fastgltf;

    auto extract_filter(fastgltf::Filter filter) -> vk::Filter
    {
        switch (filter)
        {
            case fastgltf::Filter::Nearest:
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::NearestMipMapLinear:
                return vk::Filter::eNearest;

            case fastgltf::Filter::Linear:
            case fastgltf::Filter::LinearMipMapNearest:
            case fastgltf::Filter::LinearMipMapLinear:
            default:
                return vk::Filter::eLinear;
        }
    }

    auto extract_mipmap_mode(fastgltf::Filter filter) -> vk::SamplerMipmapMode
    {
        switch (filter)
        {
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::LinearMipMapNearest:
                return vk::SamplerMipmapMode::eNearest;

            case fastgltf::Filter::NearestMipMapLinear:
            case fastgltf::Filter::LinearMipMapLinear:
            default:
                return vk::SamplerMipmapMode::eLinear;
        }
    }

    auto RendererBackend::loadImage(fastgltf::Asset& asset,
                                    fastgltf::Image& image,
                                    std::filesystem::path gltfDir) -> std::optional<Texture>
    {
        Texture newImage {};

        int width      = 0;
        int height     = 0;
        int nrChannels = 0;

        std::visit(
            fastgltf::visitor {
                [](auto&)
                {
                    MC_ASSERT_MSG(false, "Visitor did not go through all options");
                },
                [&](fastgltf::sources::URI& filePath)
                {
                    MC_ASSERT(filePath.fileByteOffset == 0);  // We don't support offsets with stbi.
                    MC_ASSERT(filePath.uri.isLocalPath());    // We're only capable of loading
                                                              // local files.
                    newImage =
                        Texture(m_device,
                                m_allocator,
                                m_commandManager,
                                std::string_view { (gltfDir.parent_path() / filePath.uri.path()).c_str() });
                },
                [&](fastgltf::sources::Vector& vector)
                {
                    unsigned char* data = stbi_load_from_memory(vector.bytes.data(),
                                                                static_cast<int>(vector.bytes.size()),
                                                                &width,
                                                                &height,
                                                                &nrChannels,
                                                                4);
                    if (data == nullptr)
                    {
                        return;
                    }

                    newImage = Texture(m_device,
                                       m_allocator,
                                       m_commandManager,
                                       vk::Extent2D { .width  = static_cast<uint32_t>(width),
                                                      .height = static_cast<uint32_t>(height) },
                                       data,
                                       width * height * 4);

                    stbi_image_free(data);
                },
                [&](fastgltf::sources::BufferView& view)
                {
                    auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                    auto& buffer     = asset.buffers[bufferView.bufferIndex];

                    std::visit(
                        fastgltf::visitor {
                            // We only care about VectorWithMime here, because we
                            // specify LoadExternalBuffers, meaning all buffers
                            // are already loaded into a vector.
                            [](auto& arg) {},
                            [&](fastgltf::sources::Array& array)
                            {
                                unsigned char* data = stbi_load_from_memory(
                                    array.bytes.data() + bufferView.byteOffset,  // NOLINT
                                    static_cast<int>(bufferView.byteLength),
                                    &width,
                                    &height,
                                    &nrChannels,
                                    4);
                                if (data != nullptr)
                                {
                                    VkExtent3D imagesize;
                                    imagesize.width  = width;
                                    imagesize.height = height;
                                    imagesize.depth  = 1;

                                    newImage = Texture(
                                        m_device,
                                        m_allocator,
                                        m_commandManager,
                                        vk::Extent2D { .width  = static_cast<uint32_t>(imagesize.width),
                                                       .height = static_cast<uint32_t>(imagesize.height) },
                                        data,
                                        width * height * 4);

                                    stbi_image_free(data);
                                }
                            } },
                        buffer.data);
                },
            }

            ,
            image.data);

        if (!newImage)
        {
            return {};
        }

        return newImage;
    }

    auto RendererBackend::loadBuffer(fastgltf::Asset& asset,
                                     fastgltf::Buffer& buffer) -> std::optional<std::vector<uint8_t>>
    {
        std::vector<uint8_t> newBuffer;

        std::visit(
            fastgltf::visitor { [](auto&)
                                {
                                    MC_ASSERT_MSG(false, "Visitor did not go through all options");
                                },
                                [&](fastgltf::sources::URI& filePath)
                                {
                                    MC_ASSERT(filePath.fileByteOffset ==
                                              0);  // We don't support offsets with stbi.
                                    MC_ASSERT(filePath.uri.isLocalPath());  // We're only capable of loading
                                                                            // local files.

                                    newBuffer = utils::readBytes<uint8_t>(filePath.uri.path());
                                },
                                [&](fastgltf::sources::Vector& vector)
                                {
                                    newBuffer = std::move(vector.bytes);
                                },
                                [&](fastgltf::sources::Array& array)
                                {
                                    newBuffer.resize(array.bytes.size());
                                    std::memcpy(newBuffer.data(), array.bytes.data(), array.bytes.size());
                                } },
            buffer.data);

        if (newBuffer.empty())
        {
            return {};
        }

        return newBuffer;
    }

    void RendererBackend::processGltf()
    {
        fs::path gltfFile = "../../khrSampleModels/2.0/DragonAttenuation/glTF/DragonAttenuation.gltf";
        // fs::path gltfFile = "../../khrSampleModels/2.0/Duck/glTF/Duck.gltf";

        MC_ASSERT_MSG(fs::exists(gltfFile), std::format("glTF file {} not found", gltfFile.c_str()));

        fg::Parser parser;

        fg::GltfDataBuffer buffer;
        buffer.loadFromFile(gltfFile);

        GltfSceneResources resources;

        fg::Asset gltf;

        Timer timer;

        fg::Expected<fg::Asset> expected = parser.loadGltfJson(
            &buffer,
            gltfFile.parent_path(),
            fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers);

        MC_ASSERT_MSG(expected,
                      std::format("Could not parse gltf file {}: {}",
                                  gltfFile.root_name().c_str(),
                                  magic_enum::enum_name(expected.error())));

        logger::info("Parsed {} successfully! ({})", gltfFile.root_name().c_str(), timer.getTotalTime());

        gltf = std::move(expected.get());

        // TODO(aether) this is a guess
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
            { vk::DescriptorType::eCombinedImageSampler, 3 },
            { vk::DescriptorType::eUniformBuffer,        3 },
            { vk::DescriptorType::eStorageBuffer,        1 }
        };

        resources.descriptorAllocator.init(m_device, gltf.materials.size(), sizes);

        resources.samplers.reserve(gltf.samplers.size());
        resources.textures.reserve(gltf.images.size());
        resources.buffers.reserve(gltf.buffers.size());
        resources.gpuBuffers.reserve(gltf.bufferViews.size());
        resources.meshDraws.reserve(gltf.meshes.size());

        timer.reset();

        for (fastgltf::Sampler& sampler : gltf.samplers)
        {
            resources.samplers.push_back(
                m_device->createSampler(
                    vk::SamplerCreateInfo()
                        .setMinLod(0)
                        .setMaxLod(vk::LodClampNone)
                        .setMinFilter(extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)))
                        .setMagFilter(extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)))
                        .setMipmapMode(
                            extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest)))) >>
                ResultChecker());
        }

        for (fastgltf::Image& image : gltf.images)
        {
            std::optional<Texture> texture = loadImage(gltf, image, gltfFile);

            MC_ASSERT_MSG(
                texture.has_value(),
                std::format("Failed to load texture '{}' for gltf file '{}'", image.name, gltfFile.c_str()));

            resources.textures.push_back(std::move(*texture));
        }

        for (fastgltf::Buffer& buffer : gltf.buffers)
        {
            std::optional<std::vector<uint8_t>> buf = loadBuffer(gltf, buffer);

            MC_ASSERT_MSG(
                buf.has_value(),
                std::format("Failed to load buffer '{}' for gltf file '{}'", buffer.name, gltfFile.c_str()));

            resources.buffers.push_back(std::move(*buf));
        }

        for (fastgltf::BufferView& buffer : gltf.bufferViews)
        {
            resources.gpuBuffers.push_back(createGPUOnlyBuffer(
                m_device,
                m_allocator,
                m_commandManager,
                vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer,
                buffer.byteLength,
                resources.buffers[buffer.bufferIndex].data() + buffer.byteOffset));
        }

        logger::info("Uploaded gltf resources in {}", timer.getTotalTime());

        MC_ASSERT_MSG(gltf.defaultScene, "Need a default scene");

        fastgltf::Scene& scene = gltf.scenes[*gltf.defaultScene];
        size_t nodeCount       = scene.nodeIndices.size();

        // wha
        std::vector<int32_t> nodeParents(nodeCount);
        std::vector<uint32_t> nodeStack(nodeCount);
        std::vector<glm::mat4> nodeMatrices(nodeCount);

        for (uint32_t i : vi::iota(0u, nodeCount))
        {
            size_t rootNode       = scene.nodeIndices[i];
            nodeParents[rootNode] = -1;
            nodeStack[i]          = rootNode;
        }

        timer.reset();

        uint64_t numNodes = 0;

        while (!nodeStack.empty())
        {
            ++numNodes;

            uint32_t nodeIndex = nodeStack.back();
            nodeStack.pop_back();
            fastgltf::Node& node = gltf.nodes[nodeIndex];

            glm::mat4 localMatrix;

            std::visit(fastgltf::visitor { [](auto&)
                                           {
                                               MC_ASSERT_MSG(false, "Visitor did not go through all options");
                                           },
                                           [&](fastgltf::TRS& trs)
                                           {
                                               glm::quat rotation    = glm::make_quat(trs.rotation.data());
                                               glm::vec3 scale       = glm::make_vec3(trs.scale.data());
                                               glm::vec3 translation = glm::make_vec3(trs.translation.data());

                                               localMatrix = glm::translate(translation) *
                                                             glm::toMat4(rotation) * glm::scale(scale);
                                           },
                                           [&](fastgltf::Node::TransformMatrix& mat)
                                           {
                                               localMatrix = glm::make_mat4(mat.data());
                                           } },
                       node.transform);

            nodeMatrices[nodeIndex] = localMatrix;

            // Iterate over the children and add them to the node stack
            // Also set their parent
            for (uint32_t childIndex : vi::iota(0u, node.children.size()))
            {
                uint32_t childNodeIndex     = node.children[childIndex];
                nodeParents[childNodeIndex] = nodeIndex;
                nodeStack.push_back(childNodeIndex);
            }

            if (!node.meshIndex)
            {
                logger::warn("Mesh index isn't present");

                continue;
            }

            glm::mat4 finalMatrix = localMatrix;
            int32_t nodeParent    = nodeParents[nodeIndex];

            // Multiply with every ancestor's model matrix
            while (nodeParent != -1)
            {
                finalMatrix = nodeMatrices[nodeParent] * finalMatrix;
                nodeParent  = nodeParents[nodeParent];
            }

            fastgltf::Mesh& mesh = gltf.meshes[*node.meshIndex];

            for (fastgltf::Primitive& primitive : mesh.primitives)
            {
                MeshDraw draw {};

                draw.materialData.model = finalMatrix;

                fastgltf::Accessor& indicesAccessor = gltf.accessors[*primitive.indicesAccessor];

                switch (indicesAccessor.componentType)
                {
                    case fastgltf::ComponentType::UnsignedInt:
                        draw.indexType = vk::IndexType::eUint32;
                        break;
                    case fastgltf::ComponentType::UnsignedShort:
                        draw.indexType = vk::IndexType::eUint16;
                        break;
                    default:
                        MC_ASSERT_MSG(false, "Unsupported index component type");
                }

                BasicBuffer* buffer = &resources.gpuBuffers[*indicesAccessor.bufferViewIndex];

                draw.indexBuffer = buffer;
                draw.indexOffset = indicesAccessor.byteOffset;
                draw.count       = indicesAccessor.count;

                MC_ASSERT_MSG(draw.count % 3 == 0, "Can only draw triangle lists");

                fastgltf::BufferView& indexBufferView = gltf.bufferViews[*indicesAccessor.bufferViewIndex];

                auto* positionAccessorIndex = primitive.findAttribute("POSITION");
                auto* normalAccessorIndex   = primitive.findAttribute("NORMAL");
                auto* tangentAccessorIndex  = primitive.findAttribute("TANGENT");
                auto* texcoordAccessorIndex = primitive.findAttribute("TEXCOORD_0");

                uint32_t* indexData32 = reinterpret_cast<uint32_t*>(
                    resources.buffers[indexBufferView.bufferIndex].data() + indexBufferView.byteOffset);
                uint16_t* indexData16 = reinterpret_cast<uint16_t*>(indexData32);

                glm::vec3* positionData = nullptr;
                uint32_t vertexCount    = 0;

                // ******** POSITION ********
                {
                    // The glTF spec guarantees that every primitive will have a position
                    // We dont need to check if its null
                    auto& accessor = gltf.accessors[positionAccessorIndex->second];

                    fastgltf::BufferView& bufferView = gltf.bufferViews[*accessor.bufferViewIndex];
                    BasicBuffer* gpuBuffer           = &resources.gpuBuffers[*accessor.bufferViewIndex];

                    vertexCount = accessor.count;

                    draw.positionBuffer = gpuBuffer;
                    draw.positionOffset = accessor.byteOffset;

                    positionData = reinterpret_cast<glm::vec3*>(
                        resources.buffers[bufferView.bufferIndex].data() + bufferView.byteOffset);
                }

                // ******** NORMALS ********
                if (normalAccessorIndex != primitive.attributes.end())
                {
                    auto& accessor = gltf.accessors[normalAccessorIndex->second];

                    BasicBuffer* gpuBuffer = &resources.gpuBuffers[*accessor.bufferViewIndex];

                    draw.normalBuffer = gpuBuffer;
                    draw.normalOffset = accessor.byteOffset;
                }

                // ******** NORMALS ********
                if (normalAccessorIndex != primitive.attributes.end())
                {
                    auto& accessor = gltf.accessors[normalAccessorIndex->second];

                    BasicBuffer* gpuBuffer = &resources.gpuBuffers[*accessor.bufferViewIndex];

                    draw.normalBuffer = gpuBuffer;
                    draw.normalOffset = accessor.byteOffset;
                }
                else
                {
                    logger::warn("Generated normals at runtime for gltf file '{}'", gltfFile.c_str());

                    std::vector<glm::vec3> normalsArray(vertexCount);

                    for (uint32_t i = 0; i < draw.count; i += 3)
                    {
                        uint32_t i0, i1, i2;

                        switch (indicesAccessor.componentType)
                        {
                            case fastgltf::ComponentType::UnsignedInt:
                                i0 = indexData32[i];
                                i1 = indexData32[i + 1];
                                i2 = indexData32[i + 2];
                                break;
                            case fastgltf::ComponentType::UnsignedShort:
                                i0 = indexData16[i];
                                i1 = indexData16[i + 1];
                                i2 = indexData16[i + 2];
                                break;
                        }

                        glm::vec3 p0 = positionData[i0];
                        glm::vec3 p1 = positionData[i1];
                        glm::vec3 p2 = positionData[i2];

                        glm::vec3 a = p1 - p0;
                        glm::vec3 b = p2 - p0;

                        glm::vec3 normal = glm::cross(a, b);

                        normalsArray[i0] += normal;
                        normalsArray[i1] += normal;
                        normalsArray[i2] += normal;
                    }

                    for (glm::vec3& normal : normalsArray)
                    {
                        normal = glm::normalize(normal);
                    }

                    draw.normalBuffer = &resources.customBuffers.emplace_back(
                        createGPUOnlyBuffer(m_device,
                                            m_allocator,
                                            m_commandManager,
                                            vk::BufferUsageFlagBits::eShaderDeviceAddress |
                                                vk::BufferUsageFlagBits::eStorageBuffer,
                                            normalsArray.size() * sizeof(decltype(normalsArray)::value_type),
                                            normalsArray.data()));

                    draw.normalOffset = 0;
                }

                // ******** TANGENTS ********
                if (tangentAccessorIndex != primitive.attributes.end())
                {
                    auto& accessor = gltf.accessors[tangentAccessorIndex->second];

                    BasicBuffer* gpuBuffer = &resources.gpuBuffers[*accessor.bufferViewIndex];

                    draw.tangentBuffer = gpuBuffer;
                    draw.tangentOffset = accessor.byteOffset;
                    draw.materialData.flags |= std::to_underlying(MaterialFeatures::TangentVertexAttribute);
                }

                // ******** TEXTURE COORDINATES ********
                if (texcoordAccessorIndex != primitive.attributes.end())
                {
                    auto& accessor = gltf.accessors[texcoordAccessorIndex->second];

                    BasicBuffer* gpuBuffer = &resources.gpuBuffers[*accessor.bufferViewIndex];

                    draw.texcoordBuffer = gpuBuffer;
                    draw.texcoordOffset = accessor.byteOffset;
                    draw.materialData.flags |= std::to_underlying(MaterialFeatures::TexcoordVertexAttribute);
                }

                MC_ASSERT_MSG(primitive.materialIndex.has_value(), "Can't load a mesh without a material");

                fastgltf::Material& material = gltf.materials[*primitive.materialIndex];

                draw.materialBuffer = BasicBuffer(m_allocator,
                                                  sizeof(Material),
                                                  vk::BufferUsageFlagBits::eUniformBuffer,
                                                  VMA_MEMORY_USAGE_CPU_TO_GPU);

                draw.descriptorSet =
                    resources.descriptorAllocator.allocate(m_device, m_materialDescriptorLayout);

                DescriptorWriter descriptorWriter;

                // TODO(aether) do something about this hardcoding
                descriptorWriter.write_buffer(
                    5, draw.materialBuffer, sizeof(Material), 0, vk::DescriptorType::eUniformBuffer);

                if (material.pbrData.baseColorTexture)
                {
                    logger::info("Using pbr data for gltf file {}!", gltfFile.c_str());

                    draw.materialData.baseColorFactor =
                        glm::make_vec4(material.pbrData.baseColorFactor.data());

                    if (std::optional<fastgltf::TextureInfo>& texInfoOpt = material.pbrData.baseColorTexture)
                    {
                        fastgltf::TextureInfo& textureInfo = *texInfoOpt;

                        fastgltf::Texture& texture = gltf.textures[textureInfo.textureIndex];
                        Texture* textureGPU        = &resources.textures[*texture.imageIndex];

                        vk::Sampler sampler = texture.samplerIndex.has_value()
                                                  ? resources.samplers[*texture.samplerIndex]
                                                  : m_dummySampler;

                        descriptorWriter.write_image(0,
                                                     textureGPU->getImageView(),
                                                     sampler,
                                                     vk::ImageLayout::eReadOnlyOptimal,
                                                     vk::DescriptorType::eCombinedImageSampler);

                        draw.materialData.flags |= std::to_underlying(MaterialFeatures::ColorTexture);
                    }
                    else
                    {
                        descriptorWriter.write_image(0,
                                                     m_dummyTexture.getImageView(),
                                                     m_dummySampler,
                                                     vk::ImageLayout::eReadOnlyOptimal,
                                                     vk::DescriptorType::eCombinedImageSampler);
                    }

                    if (std::optional<fastgltf::TextureInfo>& texInfoOpt =
                            material.pbrData.metallicRoughnessTexture)
                    {
                        fastgltf::TextureInfo& textureInfo = *texInfoOpt;

                        fastgltf::Texture& texture = gltf.textures[textureInfo.textureIndex];
                        Texture* textureGPU        = &resources.textures[*texture.imageIndex];

                        vk::Sampler sampler = texture.samplerIndex.has_value()
                                                  ? resources.samplers[*texture.samplerIndex]
                                                  : m_dummySampler;

                        descriptorWriter.write_image(1,
                                                     textureGPU->getImageView(),
                                                     sampler,
                                                     vk::ImageLayout::eReadOnlyOptimal,
                                                     vk::DescriptorType::eCombinedImageSampler);

                        draw.materialData.flags |= std::to_underlying(MaterialFeatures::RoughnessTexture);
                    }
                    else
                    {
                        descriptorWriter.write_image(1,
                                                     m_dummyTexture.getImageView(),
                                                     m_dummySampler,
                                                     vk::ImageLayout::eReadOnlyOptimal,
                                                     vk::DescriptorType::eCombinedImageSampler);
                    }

                    draw.materialData.metallicFactor  = material.pbrData.metallicFactor;
                    draw.materialData.roughnessFactor = material.pbrData.roughnessFactor;
                }

                if (auto& occlTexOpt = material.occlusionTexture)
                {
                    fastgltf::OcclusionTextureInfo& textureInfo = *occlTexOpt;
                    fastgltf::Texture& texture                  = gltf.textures[textureInfo.textureIndex];

                    // Could be the same as the roughness texture
                    Texture* textureGPU = &resources.textures[*texture.imageIndex];

                    vk::Sampler sampler = texture.samplerIndex.has_value()
                                              ? resources.samplers[*texture.samplerIndex]
                                              : m_dummySampler;

                    draw.materialData.occlusionFactor = textureInfo.strength;

                    descriptorWriter.write_image(2,
                                                 textureGPU->getImageView(),
                                                 sampler,
                                                 vk::ImageLayout::eReadOnlyOptimal,
                                                 vk::DescriptorType::eCombinedImageSampler);

                    draw.materialData.flags |= std::to_underlying(MaterialFeatures::OcclusionTexture);
                }
                else
                {
                    draw.materialData.occlusionFactor = 1.0f;

                    descriptorWriter.write_image(2,
                                                 m_dummyTexture.getImageView(),
                                                 m_dummySampler,
                                                 vk::ImageLayout::eReadOnlyOptimal,
                                                 vk::DescriptorType::eCombinedImageSampler);
                }

                draw.materialData.emissiveFactor = glm::make_vec3(material.emissiveFactor.data());

                if (std::optional<fastgltf::TextureInfo>& texInfoOpt = material.emissiveTexture)
                {
                    fastgltf::TextureInfo& textureInfo = *texInfoOpt;

                    fastgltf::Texture& texture = gltf.textures[textureInfo.textureIndex];

                    // Could be the same as the roughness texture
                    Texture* textureGPU = &resources.textures[*texture.imageIndex];

                    vk::Sampler sampler = texture.samplerIndex.has_value()
                                              ? resources.samplers[*texture.samplerIndex]
                                              : m_dummySampler;

                    descriptorWriter.write_image(3,
                                                 textureGPU->getImageView(),
                                                 sampler,
                                                 vk::ImageLayout::eReadOnlyOptimal,
                                                 vk::DescriptorType::eCombinedImageSampler);

                    draw.materialData.flags |= std::to_underlying(MaterialFeatures::EmissiveTexture);
                }
                else
                {
                    descriptorWriter.write_image(3,
                                                 m_dummyTexture.getImageView(),
                                                 m_dummySampler,
                                                 vk::ImageLayout::eReadOnlyOptimal,
                                                 vk::DescriptorType::eCombinedImageSampler);
                }

                // TODO(aether) maybe check this in the part where we create normals at runtime to save some computation?
                if (std::optional<fastgltf::NormalTextureInfo>& texInfoOpt = material.normalTexture)
                {
                    fastgltf::NormalTextureInfo& textureInfo = *texInfoOpt;

                    fastgltf::Texture& texture = gltf.textures[textureInfo.textureIndex];

                    Texture* textureGPU = &resources.textures[*texture.imageIndex];

                    vk::Sampler sampler = texture.samplerIndex.has_value()
                                              ? resources.samplers[*texture.samplerIndex]
                                              : m_dummySampler;

                    descriptorWriter.write_image(4,
                                                 textureGPU->getImageView(),
                                                 sampler,
                                                 vk::ImageLayout::eReadOnlyOptimal,
                                                 vk::DescriptorType::eCombinedImageSampler);

                    draw.materialData.flags |= std::to_underlying(MaterialFeatures::EmissiveTexture);
                }
                else
                {
                    descriptorWriter.write_image(4,
                                                 m_dummyTexture.getImageView(),
                                                 m_dummySampler,
                                                 vk::ImageLayout::eReadOnlyOptimal,
                                                 vk::DescriptorType::eCombinedImageSampler);
                }

                std::memcpy(draw.materialBuffer.getMappedData(), &draw.materialData, sizeof(MaterialData));
                descriptorWriter.update_set(m_device, draw.descriptorSet);
            }
        }

        logger::info("Finished preparing draw data in {} ({} nodes)", timer.getTotalTime(), numNodes);
    }

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
                        vk::Format::eR16G16B16A16Sfloat,
                        m_device.getMaxUsableSampleCount(),
                        vk::ImageUsageFlagBits::eTransferSrc |
                            vk::ImageUsageFlagBits::eTransferDst |  // maybe remove?
                            vk::ImageUsageFlagBits::eColorAttachment,
                        vk::ImageAspectFlagBits::eColor },

          m_drawImageResolve { m_device,
                               m_allocator,
                               m_drawImage.getDimensions(),
                               m_drawImage.getFormat(),
                               vk::SampleCountFlagBits::e1,
                               vk::ImageUsageFlagBits::eColorAttachment |
                                   vk::ImageUsageFlagBits::eTransferSrc |
                                   vk::ImageUsageFlagBits::eTransferDst,
                               vk::ImageAspectFlagBits::eColor },

          m_depthImage { m_device,
                         m_allocator,
                         m_drawImage.getDimensions(),
                         kDepthStencilFormat,
                         m_device.getMaxUsableSampleCount(),
                         vk::ImageUsageFlagBits::eDepthStencilAttachment,
                         vk::ImageAspectFlagBits::eDepth }
    // clang_format on
    {
        initImgui(window.getHandle());

        // Create dummy samplers

        m_dummySampler = m_device->createSampler({
                             .magFilter        = vk::Filter::eNearest,
                             .minFilter        = vk::Filter::eNearest,
                             .mipmapMode       = vk::SamplerMipmapMode::eLinear,
                             .addressModeU     = vk::SamplerAddressMode::eRepeat,
                             .addressModeV     = vk::SamplerAddressMode::eRepeat,
                             .addressModeW     = vk::SamplerAddressMode::eRepeat,
                             .mipLodBias       = 0.0f,
                             .anisotropyEnable = false,
                             .maxAnisotropy    = m_device.getDeviceProperties().limits.maxSamplerAnisotropy,
                             .compareEnable    = false,
                             .minLod           = 0.0f,
                             .maxLod           = 1,
                             .borderColor      = vk::BorderColor::eIntOpaqueBlack,
                             .unnormalizedCoordinates = false,
                         }) >>
                         ResultChecker();

        {
            uint32_t zero = 0;

            m_dummyTexture = Texture(
                m_device, m_allocator, m_commandManager, vk::Extent2D { 1, 1 }, &zero, sizeof(uint32_t));
        }

        m_gpuSceneDataBuffer = BasicBuffer(m_allocator,
                                           sizeof(GPUSceneData),
                                           vk::BufferUsageFlagBits::eUniformBuffer,
                                           VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_lightDataBuffer = BasicBuffer(
            m_allocator, sizeof(Light), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        initDescriptors();

        auto pipelineLayoutConfig =
            PipelineLayoutConfig()
                .setDescriptorSetLayouts({ m_sceneDataDescriptorLayout })
                .setPushConstantSettings(sizeof(GPUDrawPushConstants), vk::ShaderStageFlagBits::eVertex);

        m_texturelessPipelineLayout = PipelineLayout(m_device, pipelineLayoutConfig);

        pipelineLayoutConfig.setDescriptorSetLayouts(
            { m_sceneDataDescriptorLayout, m_materialDescriptorLayout });

        m_texturedPipelineLayout = PipelineLayout(m_device, pipelineLayoutConfig);

        processGltf();
        return;

        {
            auto pipelineConfig =
                GraphicsPipelineConfig()
                    .addShader("shaders/main.frag.spv", vk::ShaderStageFlagBits::eFragment, "main")
                    .addShader("shaders/main.vert.spv", vk::ShaderStageFlagBits::eVertex, "main")
                    .setColorAttachmentFormat(m_drawImage.getFormat())
                    .setDepthAttachmentFormat(kDepthStencilFormat)
                    .setDepthStencilSettings(true, vk::CompareOp::eGreaterOrEqual)
                    .setCullingSettings(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
                    .setSampleCount(m_device.getMaxUsableSampleCount())
                    .setSampleShadingSettings(true, 0.1f);

            m_texturedPipeline = GraphicsPipeline(m_device, m_texturedPipelineLayout, pipelineConfig);
        }

        {
            auto pipelineConfig =
                GraphicsPipelineConfig()
                    .addShader("shaders/textureless.frag.spv", vk::ShaderStageFlagBits::eFragment, "main")
                    .addShader("shaders/main.vert.spv", vk::ShaderStageFlagBits::eVertex, "main")
                    .setColorAttachmentFormat(m_drawImage.getFormat())
                    .setDepthAttachmentFormat(kDepthStencilFormat)
                    .setDepthStencilSettings(true, vk::CompareOp::eGreaterOrEqual)
                    .setCullingSettings(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
                    .setSampleCount(m_device.getMaxUsableSampleCount());

            m_texturelessPipeline = GraphicsPipeline(
                m_device,
                m_texturelessPipelineLayout,
                GraphicsPipelineConfig()
                    .addShader("shaders/textureless.frag.spv", vk::ShaderStageFlagBits::eFragment, "main")
                    .addShader("shaders/main.vert.spv", vk::ShaderStageFlagBits::eVertex, "main")
                    .setColorAttachmentFormat(m_drawImage.getFormat())
                    .setDepthAttachmentFormat(kDepthStencilFormat)
                    .setDepthStencilSettings(true, vk::CompareOp::eGreaterOrEqual)
                    .setCullingSettings(vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
                    .setSampleCount(m_device.getMaxUsableSampleCount()));
        }

        {
            Model sphereModel("res/models/uvsphere.obj");
            Model mainModel("res/models/cube.obj");

            auto sphereMesh = uploadMesh(m_device,
                                         m_allocator,
                                         m_commandManager,
                                         sphereModel.meshes[0].getVertices(),
                                         sphereModel.meshes[0].getIndices());

            auto modelMesh = uploadMesh(m_device,
                                        m_allocator,
                                        m_commandManager,
                                        mainModel.meshes[0].getVertices(),
                                        mainModel.meshes[0].getIndices());

            m_light = {
                .position    = { 1.5f,                  2.f,               0.f              },
                .color       = { 1.f,                   1.f,               1.f              },
                .attenuation = { .quadratic = 0.00007f, .linear = 0.0014f, .constant = 1.0f },
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

            m_renderItems.insert(std::pair {
                "model",
                RenderItem {
                            .model =
                        glm::scale(glm::identity<glm::mat4>(), glm::vec3 { 1.f / 3.f, 1.f / 3.f, 1.f / 3.f }),
                            .meshData = modelMesh,
                            .layout   = m_texturedPipelineLayout,
                            .pipeline = m_texturedPipeline,
                            }
            });

            // m_material = {
            //     .shininess = 32.f,
            // };

#if PROFILED
            for (size_t i : vi::iota(0u, utils::size(m_frameResources)))
            {
                std::string ctxName = fmt::format("Frame {}/{}", i + 1, kNumFramesInFlight);

                auto& ctx = m_frameResources[i].tracyContext;

                ctx = TracyVkContextCalibrated(
                    *m_device.getPhysical(),
                    *m_device.get(),
                    *m_device.getGraphicsQueue(),
                    *m_commandManager.getGraphicsCmdBuffer(i),
                    reinterpret_cast<PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT>(
                        vkGetInstanceProcAddr(*m_instance.get(),
                                              "vkGetPhysicalDeviceCalibratableTimeDomainsEXT")),
                    reinterpret_cast<PFN_vkGetCalibratedTimestampsEXT>(vkGetInstanceProcAddr(
                        *m_instance.get(), "vkGetPhysicalDeviceCalibratableTimeDomainsEXT")));

                TracyVkContextName(ctx, ctxName.data(), ctxName.size());
            }
#endif

            createSyncObjects();
        }
    }

    RendererBackend::~RendererBackend()
    {
        if (!*m_instance.get())
        {
            return;
        }

        m_device->waitIdle();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

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
                { vk::DescriptorType::eStorageBuffer,        4 },
                { vk::DescriptorType::eUniformBuffer,        4 },
                { vk::DescriptorType::eCombinedImageSampler, 4 },
            };

            m_descriptorAllocator = DescriptorAllocator(m_device, 10, sizes);
        }

        {
            m_sceneDataDescriptorLayout =
                DescriptorLayoutBuilder()
                    .addBinding(0, vk::DescriptorType::eUniformBuffer)
                    .addBinding(1, vk::DescriptorType::eUniformBuffer)
                    .build(m_device, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
        }

        {
            m_materialDescriptorLayout =
                DescriptorLayoutBuilder()
                    .addBinding(0, vk::DescriptorType::eCombinedImageSampler)  // diffuse
                    .addBinding(1, vk::DescriptorType::eCombinedImageSampler)  // roughness
                    .addBinding(2, vk::DescriptorType::eCombinedImageSampler)  // occlusion
                    .addBinding(3, vk::DescriptorType::eCombinedImageSampler)  // emissive
                    .addBinding(4, vk::DescriptorType::eCombinedImageSampler)  // normal
                    .addBinding(5, vk::DescriptorType::eUniformBuffer)         // material constants
                    .build(m_device, vk::ShaderStageFlagBits::eFragment);
        }

        m_sceneDataDescriptors = m_descriptorAllocator.allocate(m_device, m_sceneDataDescriptorLayout);

        {
            // Global scene data descriptor set
            DescriptorWriter writer;

            writer.write_buffer(
                0, m_gpuSceneDataBuffer, sizeof(GPUSceneData), 0, vk::DescriptorType::eUniformBuffer);
            writer.write_buffer(1, m_lightDataBuffer, sizeof(Light), 0, vk::DescriptorType::eUniformBuffer);
            writer.update_set(m_device, m_sceneDataDescriptors);
        }

        // {
        //     // Texture and material
        //     DescriptorWriter writer;
        //
        //     writer.write_image(0,
        //                        m_diffuseTexture.getImageView(),
        //                        m_diffuseTexture.getSampler(),
        //                        vk::ImageLayout::eReadOnlyOptimal,
        //                        vk::DescriptorType::eCombinedImageSampler);
        //
        //     writer.write_image(1,
        //                        m_specularTexture.getImageView(),
        //                        m_specularTexture.getSampler(),
        //                        vk::ImageLayout::eReadOnlyOptimal,
        //                        vk::DescriptorType::eCombinedImageSampler);
        //
        //     writer.write_buffer(
        //         2, m_materialDataBuffer, sizeof(Material), 0, vk::DescriptorType::eUniformBuffer);
        //
        //     writer.update_set(m_device, m_materialDescriptors);
        // }
    }

    void RendererBackend::initImgui(GLFWwindow* window)
    {
        std::array poolSizes {
            vk::DescriptorPoolSize()
                .setType(vk::DescriptorType::eCombinedImageSampler)
                .setDescriptorCount(kNumFramesInFlight),
        };

        vk::DescriptorPoolCreateInfo poolInfo {
            .flags   = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = kNumFramesInFlight,
        };

        poolInfo.setPoolSizes(poolSizes);

        m_imGuiPool = m_device->createDescriptorPool(poolInfo) >> ResultChecker();

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
            .Instance                    = *m_instance.get(),
            .PhysicalDevice              = *m_device.getPhysical(),
            .Device                      = *m_device.get(),
            .QueueFamily                 = m_device.getQueueFamilyIndices().graphicsFamily,
            .Queue                       = *m_device.getGraphicsQueue(),
            .DescriptorPool              = *m_imGuiPool,
            .MinImageCount               = kNumFramesInFlight,
            .ImageCount                  = utils::size(m_swapchain.getImageViews()),
            .MSAASamples                 = VK_SAMPLE_COUNT_1_BIT,
            .UseDynamicRendering         = true,
            .PipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo()
                                               .setColorAttachmentFormats(m_surface.getDetails().format)
                                               .setDepthAttachmentFormat(m_depthImage.getFormat()),
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

        m_timer.tick();

        float radius = 5.0f;

        m_light.position = {
            radius * glm::fastCos(glm::radians(
                         static_cast<float>(m_timer.getTotalTime<Timer::Seconds>().count()) * 90.f)),
            0,
            radius * glm::fastSin(glm::radians(
                         static_cast<float>(m_timer.getTotalTime<Timer::Seconds>().count()) * 90.f)),
        };

        for (RenderItem& item : m_renderItems |
                                    rn::views::filter(
                                        [](auto const& pair)
                                        {
                                            return pair.first == "light";
                                        }) |
                                    rn::views::values)
        {
            item.model = glm::scale(glm::identity<glm::mat4>(), { 0.25f, 0.25f, 0.25f }) *
                         glm::translate(glm::identity<glm::mat4>(), m_light.position);
        }

        updateDescriptors(cameraPos, glm::identity<glm::mat4>(), view, projection);
    }

    void RendererBackend::createSyncObjects()
    {
        for (FrameResources& frame : m_frameResources)
        {
            frame.imageAvailableSemaphore = m_device->createSemaphore({}) >> ResultChecker();
            frame.renderFinishedSemaphore = m_device->createSemaphore({}) >> ResultChecker();
            frame.inFlightFence =
                m_device->createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled)) >>
                ResultChecker();
        }
    }

    void RendererBackend::scheduleSwapchainUpdate()
    {
        m_windowResized = true;
    }

    void RendererBackend::handleSurfaceResize()
    {
        m_device->waitIdle();

        m_swapchain = Swapchain(m_device, m_surface);

        m_drawImage.resize(m_surface.getFramebufferExtent());
        m_drawImageResolve.resize(m_surface.getFramebufferExtent());
        m_depthImage.resize(m_surface.getFramebufferExtent());
    }

    void RendererBackend::updateDescriptors(glm::vec3 cameraPos,
                                            glm::mat4 model,
                                            glm::mat4 view,
                                            glm::mat4 projection)
    {
        auto& sceneUniformData = *static_cast<GPUSceneData*>(m_gpuSceneDataBuffer.getMappedData());
        auto& lightUniformData = *static_cast<Light*>(m_lightDataBuffer.getMappedData());

        sceneUniformData = GPUSceneData {
            .view              = view,
            .proj              = projection,
            .viewproj          = projection * view,
            .ambientColor      = glm::vec4(.1f),
            .cameraPos         = cameraPos,
            .sunlightDirection = glm::vec3 { -0.2f, -1.0f, -0.3f }
        };

        lightUniformData = m_light;
    }  // namespace renderer::backend
}  // namespace renderer::backend
