#pragma once

#include "buffer.hpp"
#include "descriptor.hpp"
#include "image.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <tiny_gltf.h>

namespace renderer::backend
{
    enum class MaterialFeatures : uint32_t
    {
        ColorTexture            = 1 << 0,
        NormalTexture           = 1 << 1,
        RoughnessTexture        = 1 << 2,
        OcclusionTexture        = 1 << 3,
        EmissiveTexture         = 1 << 4,
        TangentVertexAttribute  = 1 << 5,
        TexcoordVertexAttribute = 1 << 6,
    };

    struct alignas(16) Material
    {
        glm::vec4 baseColorFactor;
        glm::mat4 model;
        glm::mat4 modelInv;

        glm::vec3 emissiveFactor;
        // NOT MINE, SASCHA'S
        uint32_t baseColorTextureIndex;

        float metallicFactor;
        float roughnessFactor;
        float occlusionFactor;
        uint32_t flags;
    };

    struct alignas(16) Vertex
    {
        glm::vec3 position;
        float uv_x;
        glm::vec3 normal;
        float uv_y;
        glm::vec4 tangent;
    };

    struct Primitive
    {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t materialIndex;

        vk::DescriptorSet descriptorSet;
    };

    struct Mesh
    {
        std::vector<Primitive> primitives;
    };

    struct GltfNode
    {
        GltfNode* parent;
        std::vector<GltfNode*> children;
        Mesh mesh;
        glm::mat4 transformation;

        ~GltfNode()
        {
            for (auto& child : children)
            {
                delete child;
            }
        }
    };

    struct GltfImage
    {
        Texture texture;
        vk::DescriptorSet descriptorSet;
    };

    struct GltfTexture
    {
        uint32_t imageIndex;
    };

    struct SceneResources
    {
        GPUBuffer vertexBuffer;
        GPUBuffer indexBuffer;

        size_t indexCount;

        std::vector<GltfImage> images;
        std::vector<GltfTexture> textures;
        std::vector<Material> materials;
        std::vector<GltfNode*> nodes;

        DescriptorAllocatorGrowable descriptorAllocator;

        ~SceneResources()
        {
            for (auto node : nodes)
            {
                delete node;
            }
        };
    };
}  // namespace renderer::backend
