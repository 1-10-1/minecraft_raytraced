#pragma once

#include "allocator.hpp"
#include "command.hpp"
#include "device.hpp"
#include "ubo.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class BasicBuffer
    {
    public:
        BasicBuffer(Allocator& allocator, size_t allocSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage);
        ~BasicBuffer();

        BasicBuffer(BasicBuffer const&)                    = delete;
        auto operator=(BasicBuffer const&) -> BasicBuffer& = delete;
        auto operator=(BasicBuffer&&) -> BasicBuffer&      = delete;

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
        Allocator& m_allocator;

        VkBuffer m_buffer { VK_NULL_HANDLE };
        VmaAllocation m_allocation { nullptr };
        VmaAllocationInfo m_allocInfo {};
    };

    class StagedBuffer
    {
    public:
        StagedBuffer(Device& device,
                     CommandManager& commandManager,
                     VkBufferUsageFlags usageFlags,
                     void const* data,
                     size_t sizeInBytes);

        StagedBuffer(StagedBuffer const&)                    = delete;
        StagedBuffer(StagedBuffer&&)                         = delete;
        auto operator=(StagedBuffer const&) -> StagedBuffer& = delete;
        auto operator=(StagedBuffer&&) -> StagedBuffer&      = delete;

        ~StagedBuffer()
        {
            vkDestroyBuffer(m_device, m_bufferHandle, nullptr);
            vkFreeMemory(m_device, m_memoryHandle, nullptr);
        }

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkBuffer() const { return m_bufferHandle; }

    private:
        Device& m_device;

        VkBuffer m_bufferHandle { VK_NULL_HANDLE };
        VkDeviceMemory m_memoryHandle { VK_NULL_HANDLE };
    };

    class UniformBuffer
    {
    public:
        UniformBuffer(Device& device, CommandManager& commandController);

        UniformBuffer(UniformBuffer const&)                    = delete;
        UniformBuffer(UniformBuffer&&)                         = delete;
        auto operator=(UniformBuffer const&) -> UniformBuffer& = delete;
        auto operator=(UniformBuffer&&) -> UniformBuffer&      = delete;

        ~UniformBuffer()
        {
            vkDestroyBuffer(m_device, m_bufferHandle, nullptr);
            vkFreeMemory(m_device, m_memoryHandle, nullptr);
        }

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkBuffer() const { return m_bufferHandle; }

        void update(UniformBufferObject const& ubo);

    private:
        Device& m_device;

        void* m_bufferMapping { nullptr };

        VkBuffer m_bufferHandle { VK_NULL_HANDLE };
        VkDeviceMemory m_memoryHandle { VK_NULL_HANDLE };
    };

    class TextureBuffer
    {
    public:
        TextureBuffer(Device& device,
                      CommandManager& commandController,
                      unsigned char const* data,
                      uint64_t sizeInBytes);

        TextureBuffer(TextureBuffer const&)                    = delete;
        TextureBuffer(TextureBuffer&&)                         = delete;
        auto operator=(TextureBuffer const&) -> TextureBuffer& = delete;
        auto operator=(TextureBuffer&&) -> TextureBuffer&      = delete;

        ~TextureBuffer()
        {
            vkDestroyBuffer(m_device, m_bufferHandle, nullptr);
            vkFreeMemory(m_device, m_memoryHandle, nullptr);
        }

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkBuffer() const { return m_bufferHandle; }

        void init();

    private:
        Device& m_device;

        VkBuffer m_bufferHandle { VK_NULL_HANDLE };
        VkDeviceMemory m_memoryHandle { VK_NULL_HANDLE };
    };
}  // namespace renderer::backend
