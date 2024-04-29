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

        vmaCreateBuffer(m_allocator->get(), &bufferInfo, &vmaAllocInfo, &m_buffer, &m_allocation, &m_allocInfo) >>
            vkResultChecker;
    }

    BasicBuffer::~BasicBuffer()
    {
        if (m_buffer == nullptr)
        {
            return;
        }

        vmaDestroyBuffer(m_allocator->get(), m_buffer, m_allocation);
    }
}  // namespace renderer::backend
