#pragma once

#include "buffer.hpp"
#include "image.hpp"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>

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

    struct alignas(16) MaterialData
    {
        glm::vec4 baseColorFactor;
        glm::mat4 model;
        glm::mat4 modelInv;

        glm::vec3 emissiveFactor;
        float metallicFactor;

        float roughnessFactor;
        float occlusionFactor;
        uint32_t flags;
        uint32_t pad;
    };

    struct Mesh
    {
        constexpr static uint16_t invalidBufferIndex = std::numeric_limits<uint16_t>::max();

        uint16_t indexBuffer { invalidBufferIndex };
        uint16_t positionBuffer { invalidBufferIndex };
        uint16_t tangentBuffer { invalidBufferIndex };
        uint16_t normalBuffer { invalidBufferIndex };
        uint16_t texcoordBuffer { invalidBufferIndex };

        BasicBuffer materialBuffer;

        uint32_t materialIndex;

        // TODO(aether) these are all 0 for now
        uint32_t indexOffset;
        uint32_t positionOffset;
        uint32_t tangentOffset;
        uint32_t normalOffset;
        uint32_t texcoordOffset;

        uint32_t count;

        vk::IndexType indexType;

        vk::DescriptorSet descriptorSet;

        glm::mat4 model;
    };

    struct SceneResources
    {
        std::vector<MaterialData> materials;
        std::vector<Texture> textures;
        std::vector<Mesh> meshes;
    };
}  // namespace renderer::backend
