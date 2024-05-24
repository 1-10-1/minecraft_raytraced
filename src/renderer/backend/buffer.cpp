#include <mc/renderer/backend/buffer.hpp>
#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    BasicBuffer::BasicBuffer(Allocator& allocator,
                             size_t allocSize,
                             vk::BufferUsageFlags bufferUsage,
                             VmaMemoryUsage memoryUsage)
        : m_allocator { &allocator }
    {
        vk::BufferCreateInfo bufferInfo = {
            .size  = allocSize,
            .usage = bufferUsage,
        };

        VmaAllocationCreateInfo vmaAllocInfo = {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = memoryUsage,
        };

        MC_ASSERT(vmaCreateBuffer(*m_allocator,
                                  &static_cast<VkBufferCreateInfo&>(bufferInfo),
                                  &vmaAllocInfo,
                                  &m_buffer,
                                  &m_allocation,
                                  &m_allocInfo) == VK_SUCCESS);
    }

    BasicBuffer::~BasicBuffer()
    {
        if (m_buffer == nullptr)
        {
            return;
        }

        vmaDestroyBuffer(*m_allocator, m_buffer, m_allocation);
    }
}  // namespace renderer::backend
