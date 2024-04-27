#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/mesh_loader.hpp>
#include <mc/renderer/backend/mesh_upload.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/util.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    auto load_image(Device& device,
                    Allocator& alloc,
                    CommandManager& cmdManager,
                    fastgltf::Asset& asset,
                    fastgltf::Image& image) -> std::optional<std::shared_ptr<Texture>>
    {
        std::shared_ptr<Texture> newImage;

        int width      = 0;
        int height     = 0;
        int nrChannels = 0;

        std::visit(
            fastgltf::visitor {
                [](auto& arg) {},
                [&](fastgltf::sources::URI& filePath)
                {
                    assert(filePath.fileByteOffset == 0);  // We don't support offsets with stbi.
                    assert(filePath.uri.isLocalPath());    // We're only capable of loading
                                                           // local files.
                    newImage = std::make_shared<Texture>(device, alloc, cmdManager, filePath.uri.path());
                },
                [&](fastgltf::sources::Vector& vector)
                {
                    unsigned char* data = stbi_load_from_memory(
                        vector.bytes.data(), static_cast<int>(vector.bytes.size()), &width, &height, &nrChannels, 4);
                    if (data != nullptr)
                    {
                        newImage = std::make_shared<Texture>(device,
                                                             alloc,
                                                             cmdManager,
                                                             VkExtent2D { .width  = static_cast<uint32_t>(width),
                                                                          .height = static_cast<uint32_t>(height) },
                                                             data,
                                                             width * height * 4);

                        stbi_image_free(data);
                    }
                },
                [&](fastgltf::sources::BufferView& view)
                {
                    auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                    auto& buffer     = asset.buffers[bufferView.bufferIndex];

                    std::visit(fastgltf::visitor { // We only care about VectorWithMime here, because we
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

                                                           newImage = std::make_shared<Texture>(
                                                               device,
                                                               alloc,
                                                               cmdManager,
                                                               VkExtent2D {
                                                                   .width  = static_cast<uint32_t>(imagesize.width),
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

        // if any of the attempts to load the data failed, we havent written the image
        // so handle is null
        if (newImage == nullptr)
        {
            return {};
        }

        return newImage;
    }

    auto extract_filter(fastgltf::Filter filter) -> VkFilter
    {
        switch (filter)
        {
            // nearest samplers
            case fastgltf::Filter::Nearest:
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::NearestMipMapLinear:
                return VK_FILTER_NEAREST;

            // linear samplers
            case fastgltf::Filter::Linear:
            case fastgltf::Filter::LinearMipMapNearest:
            case fastgltf::Filter::LinearMipMapLinear:
            default:
                return VK_FILTER_LINEAR;
        }
    }

    auto extract_mipmap_mode(fastgltf::Filter filter) -> VkSamplerMipmapMode
    {
        switch (filter)
        {
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::LinearMipMapNearest:
                return VK_SAMPLER_MIPMAP_MODE_NEAREST;

            case fastgltf::Filter::NearestMipMapLinear:
            case fastgltf::Filter::LinearMipMapLinear:
            default:
                return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
    }

    // NOLINTNEXTLINE
    auto loadGltf(RendererBackend* engine, std::string_view filePath) -> std::optional<std::shared_ptr<LoadedGLTF>>
    {
        logger::debug("Loading GLTF: {}", filePath);

        std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
        scene->creator                    = engine;
        LoadedGLTF& file                  = *scene;

        fastgltf::Parser parser {};

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                     fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

        fastgltf::GltfDataBuffer data;
        data.loadFromFile(filePath);

        fastgltf::Asset gltf;

        std::filesystem::path path = filePath;

        auto type = fastgltf::determineGltfFileType(&data);

        if (type == fastgltf::GltfType::glTF)
        {
            auto load = parser.loadGltf(&data, path.parent_path(), gltfOptions);

            if (load)
            {
                gltf = std::move(load.get());
            }
            else
            {
                logger::error("Failed to load glTF: {}", fastgltf::to_underlying(load.error()));
                return {};
            }
        }
        else if (type == fastgltf::GltfType::GLB)
        {
            auto load = parser.loadGltfBinary(&data, path.parent_path(), gltfOptions);

            if (load)
            {
                gltf = std::move(load.get());
            }
            else
            {
                logger::error("Failed to load glTF: {}", fastgltf::to_underlying(load.error()));
                return {};
            }
        }
        else
        {
            logger::error("Failed to determine glTF container");

            return {};
        }

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        3},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,        1}
        };

        file.descriptorPool.init(engine->m_device, gltf.materials.size(), sizes);

        // load samplers
        for (fastgltf::Sampler& sampler : gltf.samplers)
        {
            VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
            sampl.maxLod              = VK_LOD_CLAMP_NONE;
            sampl.minLod              = 0;

            sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
            sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

            VkSampler newSampler = nullptr;
            vkCreateSampler(engine->m_device, &sampl, nullptr, &newSampler);

            file.samplers.push_back(newSampler);
        }

        // temporal arrays for all the objects to use while creating the GLTF data
        std::vector<std::shared_ptr<MeshAsset>> meshes;
        std::vector<std::shared_ptr<Node>> nodes;
        std::vector<std::shared_ptr<Texture>> images;
        std::vector<std::shared_ptr<GLTFMaterial>> materials;

        // load all textures
        images.reserve(gltf.images.size());

        // load all textures
        for (fastgltf::Image& image : gltf.images)
        {
            std::optional<std::shared_ptr<Texture>> img =
                load_image(engine->m_device, engine->m_allocator, engine->m_commandManager, gltf, image);

            if (img.has_value())
            {
                images.push_back(*img);
                file.images[image.name.c_str()] = *img;
            }
            else
            {
                // we failed to load, so lets give the slot a default white texture to not
                // completely break loading
                images.push_back(engine->m_checkboardTexture);
                logger::error("Failed to load gltf file texture: {}", image.name);
            }
        }
        // create buffer to hold the material data
        file.materialDataBuffer = BasicBuffer(engine->m_allocator,
                                              sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(),
                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                              VMA_MEMORY_USAGE_CPU_TO_GPU);

        int data_index = 0;
        auto* sceneMaterialConstants =
            static_cast<GLTFMetallic_Roughness::MaterialConstants*>(file.materialDataBuffer.getMappedData());

        for (fastgltf::Material& mat : gltf.materials)
        {
            std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
            materials.push_back(newMat);
            file.materials[mat.name.c_str()] = newMat;

            GLTFMetallic_Roughness::MaterialConstants constants {};
            constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
            constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
            constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
            constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

            constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
            constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;
            // write material parameters to buffer
            sceneMaterialConstants[data_index] = constants;  // NOLINT

            MaterialPass passType = MaterialPass::MainColor;
            if (mat.alphaMode == fastgltf::AlphaMode::Blend)
            {
                passType = MaterialPass::Transparent;
            }

            GLTFMetallic_Roughness::MaterialResources materialResources {};
            // default the material textures
            materialResources.colorImage        = engine->m_whiteImage->getImageView();
            materialResources.colorSampler      = engine->m_whiteImage->getSampler();
            materialResources.metalRoughImage   = engine->m_whiteImage->getImageView();
            materialResources.metalRoughSampler = engine->m_whiteImage->getSampler();

            // set the uniform buffer for the material data
            materialResources.dataBuffer       = file.materialDataBuffer;
            materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
            // grab textures from gltf file
            if (mat.pbrData.baseColorTexture.has_value())
            {
                size_t img     = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
                size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

                materialResources.colorImage   = images[img]->getImageView();
                materialResources.colorSampler = file.samplers[sampler];
            }
            // build material
            newMat->data = engine->m_metalRoughMaterial.write_material(
                engine->m_device, passType, materialResources, file.descriptorPool);

            data_index++;
        }

        // use the same vectors for all meshes so that the memory doesnt reallocate as
        // often
        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;

        for (fastgltf::Mesh& mesh : gltf.meshes)
        {
            std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
            meshes.push_back(newmesh);
            file.meshes[mesh.name.c_str()] = newmesh;
            newmesh->name                  = mesh.name;

            // clear the mesh arrays each mesh, we dont want to merge them by error
            indices.clear();
            vertices.clear();

            for (auto&& p : mesh.primitives)
            {
                GeoSurface newSurface;
                newSurface.startIndex = static_cast<uint32_t>(indices.size());
                newSurface.count      = static_cast<uint32_t>(gltf.accessors[p.indicesAccessor.value()].count);

                size_t initial_vtx = vertices.size();

                // load indexes
                {
                    fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                    indices.reserve(indices.size() + indexaccessor.count);

                    fastgltf::iterateAccessor<std::uint32_t>(gltf,
                                                             indexaccessor,
                                                             [&](std::uint32_t idx)
                                                             {
                                                                 indices.push_back(idx + initial_vtx);
                                                             });
                }

                // load vertex positions
                {
                    fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                    vertices.resize(vertices.size() + posAccessor.count);

                    fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf,
                                                                  posAccessor,
                                                                  [&](glm::vec3 v, size_t index)
                                                                  {
                                                                      Vertex newvtx {};
                                                                      newvtx.position               = v;
                                                                      newvtx.normal                 = { 1, 0, 0 };
                                                                      newvtx.color                  = glm::vec4 { 1.f };
                                                                      newvtx.uv_x                   = 0;
                                                                      newvtx.uv_y                   = 0;
                                                                      vertices[initial_vtx + index] = newvtx;
                                                                  });
                }

                // load vertex normals
                auto* normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf,
                                                                  gltf.accessors[(*normals).second],
                                                                  [&](glm::vec3 v, size_t index)
                                                                  {
                                                                      vertices[initial_vtx + index].normal = v;
                                                                  });
                }

                // load UVs
                auto* uv = p.findAttribute("TEXCOORD_0");
                if (uv != p.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf,
                                                                  gltf.accessors[(*uv).second],
                                                                  [&](glm::vec2 v, size_t index)
                                                                  {
                                                                      vertices[initial_vtx + index].uv_x = v.x;
                                                                      vertices[initial_vtx + index].uv_y = v.y;
                                                                  });
                }

                // load vertex colors
                auto* colors = p.findAttribute("COLOR_0");
                if (colors != p.attributes.end())
                {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf,
                                                                  gltf.accessors[(*colors).second],
                                                                  [&](glm::vec4 v, size_t index)
                                                                  {
                                                                      vertices[initial_vtx + index].color = v;
                                                                  });
                }

                if (p.materialIndex.has_value())
                {
                    newSurface.material = materials[p.materialIndex.value()];
                }
                else
                {
                    newSurface.material = materials[0];
                }

                newmesh->surfaces.push_back(newSurface);
            }

            newmesh->meshBuffers =
                uploadMesh(engine->m_device, engine->m_allocator, engine->m_commandManager, vertices, indices);
        }

        // load all nodes and their meshes
        for (fastgltf::Node& node : gltf.nodes)
        {
            std::shared_ptr<Node> newNode;

            // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
            if (node.meshIndex.has_value())
            {
                newNode                                      = std::make_shared<MeshNode>();
                dynamic_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
            }
            else
            {
                newNode = std::make_shared<Node>();
            }

            nodes.push_back(newNode);
            file.nodes[node.name.c_str()];

            std::visit(fastgltf::visitor { [&](fastgltf::Node::TransformMatrix matrix)
                                           {
                                               newNode->localTransform[0][0] = matrix[0];
                                               newNode->localTransform[0][1] = matrix[1];
                                               newNode->localTransform[0][2] = matrix[2];
                                               newNode->localTransform[0][3] = matrix[3];
                                               newNode->localTransform[1][0] = matrix[4];
                                               newNode->localTransform[1][1] = matrix[5];
                                               newNode->localTransform[1][2] = matrix[6];
                                               newNode->localTransform[1][3] = matrix[7];
                                               newNode->localTransform[2][0] = matrix[8];
                                               newNode->localTransform[2][1] = matrix[9];
                                               newNode->localTransform[2][2] = matrix[10];
                                               newNode->localTransform[2][3] = matrix[11];
                                               newNode->localTransform[3][0] = matrix[12];
                                               newNode->localTransform[3][1] = matrix[13];
                                               newNode->localTransform[3][2] = matrix[14];
                                               newNode->localTransform[3][3] = matrix[15];
                                           },
                                           [&](fastgltf::TRS transform)
                                           {
                                               glm::vec3 tl(transform.translation[0],
                                                            transform.translation[1],
                                                            transform.translation[2]);
                                               glm::quat rot(transform.rotation[3],
                                                             transform.rotation[0],
                                                             transform.rotation[1],
                                                             transform.rotation[2]);
                                               glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                                               glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                               glm::mat4 rm = glm::toMat4(rot);
                                               glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                               newNode->localTransform = tm * rm * sm;
                                           } },
                       node.transform);
        }

        // run loop again to setup transform hierarchy
        for (int i = 0; i < gltf.nodes.size(); i++)
        {
            fastgltf::Node& node             = gltf.nodes[i];
            std::shared_ptr<Node>& sceneNode = nodes[i];

            for (auto& c : node.children)
            {
                sceneNode->children.push_back(nodes[c]);
                nodes[c]->parent = sceneNode;
            }
        }

        // find the top nodes, with no parents
        for (auto& node : nodes)
        {
            if (node->parent.lock() == nullptr)
            {
                file.topNodes.push_back(node);
                node->refreshTransform(glm::mat4 { 1.f });
            }
        }

        return scene;
    }

    void LoadedGLTF::Draw(glm::mat4 const& topMatrix, DrawContext& ctx)
    {
        // create renderables from the scenenodes
        for (auto& n : topNodes)
        {
            n->Draw(topMatrix, ctx);
        }
    }

    void LoadedGLTF::clearAll()
    {
        descriptorPool.destroy_pools(creator->m_device);

        for (auto* sampler : samplers)
        {
            vkDestroySampler(creator->m_device, sampler, nullptr);
        }
    }
}  // namespace renderer::backend
