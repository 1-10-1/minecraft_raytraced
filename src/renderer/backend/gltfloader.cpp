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
    namespace fg = fastgltf;

    void RendererBackend::processGltf()
    {
#if 1
        fs::path gltfDir  = "../../khrSampleModels/2.0/Cube/glTF";
        fs::path prevPath = fs::current_path();
        fs::current_path(gltfDir);
        fs::path path = fs::current_path() / "Cube.gltf";
#elif 0
        fs::path gltfDir  = "../../khrSampleModels/2.0/AntiqueCamera/glTF";
        fs::path prevPath = fs::current_path();
        fs::current_path(gltfDir);
        fs::path path = fs::current_path() / "AntiqueCamera.gltf";

#else
        fs::path gltfDir  = "../../khrSampleModels/2.0/Sponza/glTF";
        fs::path prevPath = fs::current_path();
        fs::current_path(gltfDir);
        fs::path path = fs::current_path() / "Sponza.gltf";
#endif

        MC_ASSERT_MSG(fs::exists(path), "glTF file path does not exist: {}", path.string());

        SceneResources scene;

        fg::Parser parser;

        constexpr auto gltfOptions = fg::Options::DontRequireValidAssetMember | fg::Options::AllowDouble |
                                     fg::Options::LoadGLBBuffers | fg::Options::LoadExternalBuffers |
                                     fg::Options::LoadExternalImages | fg::Options::GenerateMeshIndices;

        auto gltfFile = fg::MappedGltfFile::FromPath(path);

        MC_ASSERT_MSG(gltfFile, "Failed to open glTF file '{}'", path.string());

        fg::Asset asset;

        {
            auto expected = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);

            MC_ASSERT_MSG(expected, "Failed to load glTF file '{}'", path.string());

            asset = std::move(expected.get());
        }

        MC_ASSERT(scene.materials.size() == 0);

        auto& defaultMaterial           = scene.materials.emplace_back();
        defaultMaterial.baseColorFactor = glm::identity<glm::vec4>();  // FIXME(aether) careful
        defaultMaterial.flags           = 0;

        for (fastgltf::Image& image : asset.images)
        {
            scene.textures.push_back(loadImage(asset, image, path.parent_path()));
        }

        for (auto& material : asset.materials)
        {
            scene.materials.push_back(loadMaterial(asset, material));
        }

        for (auto& mesh : asset.meshes)
        {
            scene.meshes.push_back(loadMesh(asset, mesh));
        }

        fs::current_path(prevPath);
    }

    auto loadMesh(fastgltf::Asset& asset, fastgltf::Mesh& mesh) -> Mesh
    {
        auto& asset  = viewer->asset;
        Mesh outMesh = {};
        outMesh.primitives.resize(mesh.primitives.size());

        for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it)
        {
            auto* positionIt = it->findAttribute("POSITION");
            assert(positionIt != it->attributes.end());  // A mesh primitive is required to hold the
                                                         // POSITION attribute.
            assert(it->indicesAccessor.has_value());     // We specify GenerateMeshIndices,
                                                         // so we should always have indices

            // Generate the VAO
            GLuint vao = GL_NONE;
            glCreateVertexArrays(1, &vao);

            std::size_t baseColorTexcoordIndex = 0;

            // Get the output primitive
            auto index              = std::distance(mesh.primitives.begin(), it);
            auto& primitive         = outMesh.primitives[index];
            primitive.primitiveType = fastgltf::to_underlying(it->type);
            primitive.vertexArray   = vao;

            if (it->materialIndex.has_value())
            {
                primitive.materialUniformsIndex =
                    it->materialIndex.value() + 1;  // Adjust for default material
                auto& material = viewer->asset.materials[it->materialIndex.value()];

                auto& baseColorTexture = material.pbrData.baseColorTexture;
                if (baseColorTexture.has_value())
                {
                    auto& texture = viewer->asset.textures[baseColorTexture->textureIndex];
                    if (!texture.imageIndex.has_value())
                        return false;
                    primitive.albedoTexture = viewer->textures[texture.imageIndex.value()].texture;

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

            {
                // Position
                auto& positionAccessor = asset.accessors[positionIt->second];
                if (!positionAccessor.bufferViewIndex.has_value())
                    continue;

                // Create the vertex buffer for this primitive, and use the accessor tools
                // to copy directly into the mapped buffer.
                glCreateBuffers(1, &primitive.vertexBuffer);
                glNamedBufferData(
                    primitive.vertexBuffer, positionAccessor.count * sizeof(Vertex), nullptr, GL_STATIC_DRAW);
                auto* vertices =
                    static_cast<Vertex*>(glMapNamedBuffer(primitive.vertexBuffer, GL_WRITE_ONLY));
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset,
                    positionAccessor,
                    [&](fastgltf::math::fvec3 pos, std::size_t idx)
                    {
                        vertices[idx].position = fastgltf::math::fvec3(pos.x(), pos.y(), pos.z());
                        vertices[idx].uv       = fastgltf::math::fvec2();
                    });
                glUnmapNamedBuffer(primitive.vertexBuffer);

                glEnableVertexArrayAttrib(vao, 0);
                glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
                glVertexArrayAttribBinding(vao, 0, 0);

                glVertexArrayVertexBuffer(vao, 0, primitive.vertexBuffer, 0, sizeof(Vertex));
            }

            auto texcoordAttribute = std::string("TEXCOORD_") + std::to_string(baseColorTexcoordIndex);
            if (auto const* texcoord = it->findAttribute(texcoordAttribute); texcoord != it->attributes.end())
            {
                // Tex coord
                auto& texCoordAccessor = asset.accessors[texcoord->second];
                if (!texCoordAccessor.bufferViewIndex.has_value())
                    continue;

                auto* vertices =
                    static_cast<Vertex*>(glMapNamedBuffer(primitive.vertexBuffer, GL_WRITE_ONLY));
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                    asset,
                    texCoordAccessor,
                    [&](fastgltf::math::fvec2 uv, std::size_t idx)
                    {
                        vertices[idx].uv = fastgltf::math::fvec2(uv.x(), uv.y());
                    });
                glUnmapNamedBuffer(primitive.vertexBuffer);

                glEnableVertexArrayAttrib(vao, 1);
                glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 0);
                glVertexArrayAttribBinding(vao, 1, 1);

                glVertexArrayVertexBuffer(
                    vao, 1, primitive.vertexBuffer, offsetof(Vertex, uv), sizeof(Vertex));
            }

            // Generate the indirect draw command
            auto& draw         = primitive.draw;
            draw.instanceCount = 1;
            draw.baseInstance  = 0;
            draw.baseVertex    = 0;
            draw.firstIndex    = 0;

            auto& indexAccessor = asset.accessors[it->indicesAccessor.value()];
            if (!indexAccessor.bufferViewIndex.has_value())
                return false;
            draw.count = static_cast<std::uint32_t>(indexAccessor.count);

            // Create the index buffer and copy the indices into it.
            glCreateBuffers(1, &primitive.indexBuffer);
            if (indexAccessor.componentType == fastgltf::ComponentType::UnsignedByte ||
                indexAccessor.componentType == fastgltf::ComponentType::UnsignedShort)
            {
                primitive.indexType = GL_UNSIGNED_SHORT;
                glNamedBufferData(primitive.indexBuffer,
                                  static_cast<GLsizeiptr>(indexAccessor.count * sizeof(std::uint16_t)),
                                  nullptr,
                                  GL_STATIC_DRAW);
                auto* indices =
                    static_cast<std::uint16_t*>(glMapNamedBuffer(primitive.indexBuffer, GL_WRITE_ONLY));
                fastgltf::copyFromAccessor<std::uint16_t>(asset, indexAccessor, indices);
                glUnmapNamedBuffer(primitive.indexBuffer);
            }
            else
            {
                primitive.indexType = GL_UNSIGNED_INT;
                glNamedBufferData(primitive.indexBuffer,
                                  static_cast<GLsizeiptr>(indexAccessor.count * sizeof(std::uint32_t)),
                                  nullptr,
                                  GL_STATIC_DRAW);
                auto* indices =
                    static_cast<std::uint32_t*>(glMapNamedBuffer(primitive.indexBuffer, GL_WRITE_ONLY));
                fastgltf::copyFromAccessor<std::uint32_t>(asset, indexAccessor, indices);
                glUnmapNamedBuffer(primitive.indexBuffer);
            }

            glVertexArrayElementBuffer(vao, primitive.indexBuffer);
        }

        // Create the buffer holding all of our primitive structs.
        glCreateBuffers(1, &outMesh.drawsBuffer);
        glNamedBufferData(outMesh.drawsBuffer,
                          static_cast<GLsizeiptr>(outMesh.primitives.size() * sizeof(Primitive)),
                          outMesh.primitives.data(),
                          GL_STATIC_DRAW);
    };

    auto loadMaterial(fastgltf::Asset& asset, fastgltf::Material& material) -> MaterialData
    {
        // TODO(aether) incomplete asf

        return { .baseColorFactor = glm::make_vec4(material.pbrData.baseColorFactor.data()),
                 .flags           = (material.pbrData.baseColorTexture.has_value()
                                         ? (std::to_underlying(MaterialFeatures::ColorTexture))
                                         : 0) };
    };

    auto
    RendererBackend::loadImage(fastgltf::Asset& asset, fastgltf::Image& image, fs::path gltfDir) -> Texture
    {
        Texture texture {};

        int width      = 0;
        int height     = 0;
        int nrChannels = 0;

        std::visit(
            fastgltf::visitor {
                [](auto&)
                {
                    logger::warn("Visitor did not go through all options");
                },
                [&](fastgltf::sources::URI& filePath)
                {
                    MC_ASSERT(filePath.fileByteOffset == 0);  // We don't support offsets with stbi.
                    MC_ASSERT(filePath.uri.isLocalPath());    // We're only capable of loading
                                                              // local files.
                    texture =
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

                    texture = Texture(m_device,
                                      m_allocator,
                                      m_commandManager,
                                      vk::Extent2D { .width  = static_cast<uint32_t>(width),
                                                     .height = static_cast<uint32_t>(height) },
                                      data,
                                      width * height * 4);

                    stbi_image_free(data);
                },
                [&](fastgltf::sources::Array& array)
                {
                    unsigned char* data = stbi_load_from_memory(array.bytes.data(),
                                                                static_cast<int>(array.bytes.size()),
                                                                &width,
                                                                &height,
                                                                &nrChannels,
                                                                4);
                    if (data == nullptr)
                    {
                        return;
                    }

                    texture = Texture(m_device,
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

                                    texture = Texture(
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
            },
            image.data);

        MC_ASSERT(texture);

        return texture;
    }
}  // namespace renderer::backend
