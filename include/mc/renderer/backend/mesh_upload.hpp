#pragma once

#include "allocator.hpp"
#include "device.hpp"
#include "mesh_buffers.hpp"
#include "vertex.hpp"
#include <span>

namespace renderer::backend
{
    auto uploadMesh(Device& device,
                    Allocator& allocator,
                    CommandManager const& cmdManager,
                    std::span<Vertex> vertices,
                    std::span<uint32_t> indices) -> GPUMeshBuffers;
}  // namespace renderer::backend
