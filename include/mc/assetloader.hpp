#pragma once

#include <string>
#include <vector>

#include <assimp/scene.h>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

enum class TextureType
{
    Diffuse,
    Specular,
    Height,
    Normal
};

struct VertexData
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct TextureData
{
    TextureType type;
    std::string path;
};

class Mesh
{
public:
    Mesh(std::vector<VertexData>&& vertices,
         std::vector<unsigned int>&& indices,
         std::vector<TextureData>&& textures);

    [[nodiscard]] auto getVertices() const -> std::vector<VertexData> const& { return m_vertices; }

    [[nodiscard]] auto getIndices() const -> std::vector<uint32_t> const& { return m_indices; }

    [[nodiscard]] auto getTextures() const -> std::vector<TextureData> const& { return m_textures; }

private:
    void setupMesh();

    std::vector<VertexData> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<TextureData> m_textures;
};

class Model
{
public:
    explicit Model(std::string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    std::vector<TextureData> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;

private:
    void loadModel(std::string const& path);

    void processNode(aiNode* node, aiScene const* scene);

    auto processMesh(aiMesh* mesh, aiScene const* scene) -> Mesh;

    auto loadMaterialTextures(aiMaterial* mat, aiTextureType type, TextureType typeEnum)
        -> std::vector<TextureData>;
};
