#include "mc/renderer/backend/resource.hpp"
#include <mc/renderer/backend/buffer.hpp>
#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    BasicBuffer::BasicBuffer(Allocator& allocator,
                             size_t allocSize,
                             VkBufferUsageFlags bufferUsage,
                             VmaMemoryUsage memoryUsage)
        : m_allocator { &allocator }
    {
        VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = allocSize,
            .usage = bufferUsage,
        };

        VmaAllocationCreateInfo vmaAllocInfo = {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = memoryUsage,
        };

        VkBuffer buffer;

        vmaCreateBuffer(*m_allocator, &bufferInfo, &vmaAllocInfo, &buffer, &m_allocation, &m_allocInfo) >>
            vkResultChecker;

        m_resource = VulkanResource<VkBuffer>(*m_allocator, m_allocation, buffer);
    }
}  // namespace renderer::backend
