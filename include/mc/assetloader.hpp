#pragma once

#include "renderer/backend/vertex.hpp"

#include <string>
#include <vector>

#include <assimp/scene.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

using renderer::backend::Vertex;

enum class TextureType
{
    Diffuse,
    Specular,
    Height,
    Normal
};

struct TextureData
{
    TextureType type;
    std::string path;
};

class Mesh
{
public:
    Mesh(std::vector<Vertex>&& vertices, std::vector<unsigned int>&& indices, std::vector<TextureData>&& textures);

    [[nodiscard]] auto getVertices() -> std::vector<Vertex>& { return m_vertices; }

    [[nodiscard]] auto getIndices() -> std::vector<uint32_t>& { return m_indices; }

    [[nodiscard]] auto getTextures() const -> std::vector<TextureData> const& { return m_textures; }

private:
    void setupMesh();

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<TextureData> m_textures;
};

class Model
{
public:
    explicit Model(std::string const& path, bool gamma = false) : gammaCorrection(gamma) { loadModel(path); }

    std::vector<TextureData> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;

private:
    void loadModel(std::string const& path);

    void processNode(aiNode* node, aiScene const* scene);

    auto processMesh(aiMesh* mesh, aiScene const* scene) -> Mesh;

    auto loadMaterialTextures(aiMaterial* mat, aiTextureType type, TextureType typeEnum) -> std::vector<TextureData>;
};
