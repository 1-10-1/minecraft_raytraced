#pragma once

#include "command.hpp"
#include "image.hpp"
#include "material.hpp"
#include "mesh_buffers.hpp"

#include <optional>
#include <string>
#include <vector>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    struct RenderObject
    {
        uint32_t indexCount;
        uint32_t firstIndex;
        VkBuffer indexBuffer;

        MaterialInstance* material;

        glm::mat4 transform;
        VkDeviceAddress vertexBufferAddress;
    };

    struct DrawContext
    {
        std::vector<RenderObject> OpaqueSurfaces;
    };

    // base class for a renderable dynamic object
    class IRenderable
    {
        virtual void Draw(glm::mat4 const& topMatrix, DrawContext& ctx) = 0;

    public:
        virtual ~IRenderable() = default;
    };

    // implementation of a drawable scene node.
    // the scene node can hold children and will also keep a transform to propagate
    // to them
    struct Node : public IRenderable
    {
        // parent pointer must be a weak pointer to avoid circular dependencies
        std::weak_ptr<Node> parent;
        std::vector<std::shared_ptr<Node>> children;

        glm::mat4 localTransform;
        glm::mat4 worldTransform;

        void refreshTransform(glm::mat4 const& parentMatrix)
        {
            worldTransform = parentMatrix * localTransform;
            for (auto& c : children)
            {
                c->refreshTransform(worldTransform);
            }
        }

        void Draw(glm::mat4 const& topMatrix, DrawContext& ctx) override
        {
            for (auto& c : children)
            {
                c->Draw(topMatrix, ctx);
            }
        }
    };

    struct GLTFMaterial
    {
        MaterialInstance data;
    };

    struct GeoSurface
    {
        uint32_t startIndex;
        uint32_t count;
        std::shared_ptr<GLTFMaterial> material;
    };

    struct MeshAsset
    {
        std::string name;

        std::vector<GeoSurface> surfaces;
        GPUMeshBuffers meshBuffers;
    };

    struct MeshNode : public Node
    {
        std::shared_ptr<MeshAsset> mesh {};

        void Draw(glm::mat4 const& topMatrix, DrawContext& ctx) override;
    };

    struct LoadedGLTF : public IRenderable  // NOLINT
    {
        // storage for all the data on a given glTF file
        std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
        std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
        std::unordered_map<std::string, VkImageView> images;
        std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

        // nodes that dont have a parent, for iterating through the file in tree order
        std::vector<std::shared_ptr<Node>> topNodes;

        std::vector<VkSampler> samplers;

        DescriptorAllocatorGrowable descriptorPool;

        BasicBuffer materialDataBuffer;

        RendererBackend* creator;

        ~LoadedGLTF() override { clearAll(); };

        void Draw(glm::mat4 const& topMatrix, DrawContext& ctx) override;

    private:
        void clearAll();
    };

    // ****************************************************************************************************************
}  // namespace renderer::backend
