#include <mc/renderer/backend/buffer.hpp>
#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>
#include <vulkan/vulkan_core.h>

namespace
{
    using namespace renderer::backend;

    struct BufferHandles
    {
        VkBuffer buffer { VK_NULL_HANDLE };
        VkDeviceMemory memory { VK_NULL_HANDLE };
    };

    [[nodiscard]] auto
    createBuffer(Device const& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        -> BufferHandles
    {
        BufferHandles handles {};
        auto& [buffer, memory] = handles;

        auto const& queueFamiliesIndices { device.getQueueFamilyIndices() };

        std::array indices { queueFamiliesIndices.graphicsFamily.value(), queueFamiliesIndices.transferFamily.value() };

        VkBufferCreateInfo bufferInfo { .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                        .size                  = size,
                                        .usage                 = usage,
                                        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                                        .queueFamilyIndexCount = utils::size(indices),
                                        .pQueueFamilyIndices   = indices.data() };

        vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) >> vkResultChecker;

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        VkMemoryAllocateInfo allocInfo {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memoryRequirements.size,
            .memoryTypeIndex = device.findSuitableMemoryType(memoryRequirements.memoryTypeBits, properties),
        };

        vkAllocateMemory(device, &allocInfo, nullptr, &memory) >> vkResultChecker;

        vkBindBufferMemory(device, buffer, memory, 0);

        return handles;
    }

}  // namespace

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

    StagedBuffer::StagedBuffer(Device& device,
                               CommandManager& commandManager,
                               VkBufferUsageFlags usageFlags,
                               void const* data,
                               size_t sizeInBytes)
        : m_device { device }
    {
        auto [stagingBuffer, stagingBufferMemory] =
            createBuffer(m_device,
                         sizeInBytes,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* mappedData { nullptr };

        vkMapMemory(m_device, stagingBufferMemory, 0, sizeInBytes, 0, &mappedData);

        std::memcpy(mappedData, data, static_cast<size_t>(sizeInBytes));

        vkUnmapMemory(m_device, stagingBufferMemory);

        BufferHandles bufferHandles = createBuffer(
            m_device, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_bufferHandle = bufferHandles.buffer;
        m_memoryHandle = bufferHandles.memory;

        VkBufferCopy copyRegion { .size = sizeInBytes };

        vkCmdCopyBuffer(
            ScopedCommandBuffer(m_device, commandManager.getTransferCmdPool(), m_device.getTransferQueue(), true),
            stagingBuffer,
            m_bufferHandle,
            1,
            &copyRegion);

        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
    }

    UniformBuffer::UniformBuffer(Device& device, CommandManager& commandController) : m_device { device }
    {
        constexpr VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        BufferHandles handles =
            createBuffer(m_device,
                         bufferSize,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        m_bufferHandle = handles.buffer;
        m_memoryHandle = handles.memory;

        vkMapMemory(m_device, m_memoryHandle, 0, bufferSize, 0, &m_bufferMapping);
    }

    void UniformBuffer::update(UniformBufferObject const& ubo)
    {
        std::memcpy(m_bufferMapping, &ubo, sizeof(UniformBufferObject));
    }

    TextureBuffer::TextureBuffer(Device& device,
                                 CommandManager& commandController,
                                 unsigned char const* data,
                                 uint64_t sizeInBytes)
        : m_device { device }
    {
        BufferHandles handles =
            createBuffer(m_device,
                         sizeInBytes,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        m_bufferHandle = handles.buffer;
        m_memoryHandle = handles.memory;

        void* mappedData { nullptr };

        vkMapMemory(m_device, m_memoryHandle, 0, sizeInBytes, 0, &mappedData);

        std::memcpy(mappedData, data, static_cast<size_t>(sizeInBytes));

        vkUnmapMemory(m_device, m_memoryHandle);
    }
}  // namespace renderer::backend
