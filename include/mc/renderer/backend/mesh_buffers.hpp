#pragma once

#include "buffer.hpp"

namespace renderer::backend
{
    struct GPUMeshBuffers
    {
        BasicBuffer indexBuffer;
        BasicBuffer vertexBuffer;
        VkDeviceAddress vertexBufferAddress;
    };
}  // namespace renderer::backend
