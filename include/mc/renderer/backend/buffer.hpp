#pragma once

#include "allocator.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class BasicBuffer
    {
    public:
        BasicBuffer() = default;
        BasicBuffer(Allocator& allocator, size_t allocSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage);
        ~BasicBuffer();

        BasicBuffer(BasicBuffer const&)                    = delete;
        auto operator=(BasicBuffer const&) -> BasicBuffer& = delete;

        auto operator=(BasicBuffer&& other) noexcept -> BasicBuffer&
        {
            m_buffer     = other.m_buffer;
            m_allocator  = other.m_allocator;
            m_allocation = other.m_allocation;
            m_allocInfo  = other.m_allocInfo;

            other.m_buffer     = VK_NULL_HANDLE;
            other.m_allocation = nullptr;
            other.m_allocInfo  = {};

            return *this;
        }

        BasicBuffer(BasicBuffer&& other) noexcept
            : m_allocator { other.m_allocator },
              m_buffer { other.m_buffer },
              m_allocation { other.m_allocation },
              m_allocInfo { other.m_allocInfo }
        {
            other.m_buffer     = VK_NULL_HANDLE;
            other.m_allocation = nullptr;
            other.m_allocInfo  = {};
        };

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkBuffer() const { return m_buffer; }

        [[nodiscard]] auto getMappedData() const -> void* { return m_allocInfo.pMappedData; }

    private:
        Allocator* m_allocator { nullptr };

        VkBuffer m_buffer { VK_NULL_HANDLE };
        VmaAllocation m_allocation { nullptr };
        VmaAllocationInfo m_allocInfo {};
    };
}  // namespace renderer::backend
