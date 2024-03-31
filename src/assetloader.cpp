#include <mc/assetloader.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <kv/exceptions.h>
#include <kv/logger.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-float-conversion"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#pragma clang diagnostic pop

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

Mesh::Mesh(std::vector<VertexData>&& vertices, std::vector<unsigned int>&& indices, std::vector<TextureData>&& textures)
    : m_vertices { std::move(vertices) }, m_indices { std::move(indices) }, m_textures { std::move(textures) }
{
    setupMesh();
}

void Mesh::setupMesh() {}

void Model::loadModel(std::string const& path)
{
    Assimp::Importer importer;
    aiScene const* scene =
        importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if ((scene == nullptr) || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || (scene->mRootNode == nullptr))
    {
        throw Error(AssetError, fmt::format("Assimp error: {}", importer.GetErrorString()));
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
    std::vector<VertexData> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureData> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        vertices.push_back({
            .position { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z },
            .normal = mesh->HasNormals() ? glm::vec3 { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z }
             : glm::vec3 {},
 // A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
  // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            .texCoords = mesh->mTextureCoords[0] != nullptr ? glm::vec2 { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y }
                                                            : glm::vec2 {},
            .tangent   = mesh->mTextureCoords[0] != nullptr
                             ? glm::vec3 { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z }
                             : glm::vec3 {},
            .bitangent = mesh->mTextureCoords[0] != nullptr
                             ? glm::vec3 { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z }
                             : glm::vec3 {},
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

    std::vector<TextureData> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, TextureType::Specular);
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
