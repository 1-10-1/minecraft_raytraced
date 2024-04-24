#pragma once

#include "mesh_buffers.hpp"

#include <optional>
#include <string>
#include <vector>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

namespace renderer::backend
{
    struct GeoSurface
    {
        uint32_t startIndex;
        uint32_t count;
    };

    struct MeshAsset
    {
        std::string name;

        std::vector<GeoSurface> surfaces;
        GPUMeshBuffers meshBuffers;
    };

    auto loadGltfMeshes(Device& device,
                        Allocator& allocator,
                        CommandManager const& cmdManager,
                        std::filesystem::path const& filePath)
        -> std::optional<std::vector<std::shared_ptr<MeshAsset>>>;
}  // namespace renderer::backend
