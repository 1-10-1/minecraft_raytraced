#pragma once

#include "allocator.hpp"
#include "mc/utils.hpp"
#include "resource.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class BasicBuffer
    {
    public:
        BasicBuffer()  = default;
        ~BasicBuffer() = default;

        BasicBuffer(Allocator& allocator, size_t allocSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage);

        DEFAULT_MOVE(BasicBuffer);
        RESTRICT_COPY(BasicBuffer);

        [[nodiscard]] operator VkBuffer() const { return m_resource; }

        [[nodiscard]] auto getMappedData() const -> void const* { return m_allocInfo.pMappedData; }

        [[nodiscard]] auto getMappedData() -> void* { return m_allocInfo.pMappedData; }

    private:
        Allocator* m_allocator { nullptr };

        VulkanResource<VkBuffer> m_resource {};
        VmaAllocation m_allocation { nullptr };
        VmaAllocationInfo m_allocInfo {};
    };
}  // namespace renderer::backend
