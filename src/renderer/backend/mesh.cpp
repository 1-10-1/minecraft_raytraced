#include "mc/exceptions.hpp"
#include "mc/logger.hpp"
#include "mc/renderer/backend/command.hpp"
#include "mc/renderer/backend/mesh.hpp"

#include <span>
#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

namespace renderer::backend
{
    auto uploadMesh(Device& device,
                    Allocator& allocator,
                    CommandManager const& cmdManager,
                    std::span<Vertex> vertices,
                    std::span<uint32_t> indices) -> std::shared_ptr<GPUMeshData>
    {
        size_t const vertexBufferSize = vertices.size() * sizeof(Vertex);
        size_t const indexBufferSize  = indices.size() * sizeof(uint32_t);

        std::shared_ptr<GPUMeshData> newSurface { std::make_shared<GPUMeshData>() };

        newSurface->indexBuffer = BasicBuffer(allocator,
                                              indexBufferSize,
                                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                              VMA_MEMORY_USAGE_GPU_ONLY);

        newSurface->vertexBuffer = BasicBuffer(allocator,
                                               vertexBufferSize,
                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                               VMA_MEMORY_USAGE_GPU_ONLY);

        newSurface->indexCount = indices.size();

        VkBufferDeviceAddressInfo deviceAdressInfo { .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                     .buffer = newSurface->vertexBuffer };

        newSurface->vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAdressInfo);

        {
            BasicBuffer staging { allocator,
                                  vertexBufferSize + indexBufferSize,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VMA_MEMORY_USAGE_CPU_ONLY };

            void* data = staging.getMappedData();

            memcpy(data, vertices.data(), vertexBufferSize);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            memcpy(static_cast<char*>(data) + vertexBufferSize, indices.data(), indexBufferSize);

            {
                ScopedCommandBuffer cmdBuf { device, cmdManager.getTransferCmdPool(), device.getTransferQueue(), true };

                VkBufferCopy vertexCopy {
                    .srcOffset = 0,
                    .dstOffset = 0,
                    .size      = vertexBufferSize,
                };

                VkBufferCopy indexCopy {
                    .srcOffset = vertexBufferSize,
                    .dstOffset = 0,
                    .size      = indexBufferSize,
                };

                vkCmdCopyBuffer(cmdBuf, staging, newSurface->indexBuffer, 1, &indexCopy);
                vkCmdCopyBuffer(cmdBuf, staging, newSurface->vertexBuffer, 1, &vertexCopy);
            };
        }

        return newSurface;
    };
}  // namespace renderer::backend

Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<unsigned int>&& indices, std::vector<TextureData>&& textures)
    : m_vertices { std::move(vertices) }, m_indices { std::move(indices) }, m_textures { std::move(textures) }
{
    setupMesh();
}

void Model::loadModel(std::string const& path)
{
    Assimp::Importer importer;
    aiScene const* scene = importer.ReadFile(path, aiProcess_Triangulate);

    if ((scene == nullptr) || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || (scene->mRootNode == nullptr))
    {
        MC_THROW Error(AssetError, std::format("Assimp error: {}", importer.GetErrorString()));
    }

    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, aiScene const* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

auto Model::processMesh(aiMesh* mesh, aiScene const* scene) -> Mesh
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureData> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        vertices.push_back({
            .position { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z },
            .uv_x = mesh->mTextureCoords[0] != nullptr ? mesh->mTextureCoords[0][i].x : 0,
            .normal { mesh->HasNormals() ? glm::vec3 { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z }
                                         : glm::vec3 {} },
            // A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            .uv_y = mesh->mTextureCoords[0] != nullptr ? mesh->mTextureCoords[0][i].y : 0,
            .color {
                       mesh->mColors[0] != nullptr
                    ? glm::vec4 { mesh->mColors[0]->r, mesh->mColors[0]->g, mesh->mColors[0]->b, mesh->mColors[0]->a }
                    : glm::vec4 {} }
        });
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    std::vector<TextureData> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, TextureType::Diffuse);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    std::vector<TextureData> specularMaps =
        loadMaterialTextures(material, aiTextureType_SPECULAR, TextureType::Specular);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    std::vector<TextureData> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, TextureType::Normal);
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    std::vector<TextureData> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, TextureType::Height);
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    return { std::move(vertices), std::move(indices), std::move(textures) };
}

auto Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, TextureType typeEnum) -> std::vector<TextureData>
{
    std::vector<TextureData> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        bool skip = false;

        for (TextureData const& tex : textures_loaded)
        {
            if (tex.path == str.C_Str())
            {
                textures.push_back(tex);
                skip = true;
                break;
            }
        }
        if (skip)
        {
            continue;
        }

        TextureData texture {
            .type = typeEnum,
            .path = str.C_Str(),
        };

        textures.push_back(texture);
        textures_loaded.push_back(texture);
    }
    return textures;
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
