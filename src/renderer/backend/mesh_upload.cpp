#include <mc/renderer/backend/mesh_upload.hpp>

namespace renderer::backend
{
    auto uploadMesh(Device& device,
                    Allocator& allocator,
                    CommandManager const& cmdManager,
                    std::span<Vertex> vertices,
                    std::span<uint32_t> indices) -> GPUMeshBuffers
    {
        size_t const vertexBufferSize = vertices.size() * sizeof(Vertex);
        size_t const indexBufferSize  = indices.size() * sizeof(uint32_t);

        GPUMeshBuffers newSurface {
            .indexBuffer = BasicBuffer(allocator,
                                       indexBufferSize,
                                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       VMA_MEMORY_USAGE_GPU_ONLY),

            .vertexBuffer = BasicBuffer(allocator,
                                        vertexBufferSize,
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                        VMA_MEMORY_USAGE_GPU_ONLY)
        };

        VkBufferDeviceAddressInfo deviceAdressInfo { .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                     .buffer = newSurface.vertexBuffer };

        newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(device, &deviceAdressInfo);

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

                vkCmdCopyBuffer(cmdBuf, staging, newSurface.indexBuffer, 1, &indexCopy);
                vkCmdCopyBuffer(cmdBuf, staging, newSurface.vertexBuffer, 1, &vertexCopy);
            };
        }

        return newSurface;
    };
}  // namespace renderer::backend
