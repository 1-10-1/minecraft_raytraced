#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/constants.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    ScopedCommandBuffer::ScopedCommandBuffer(Device& device, VkCommandPool commandPool, VkQueue queue, bool oneTimeUse)
        : m_device { device }, m_pool { commandPool }, m_queue { queue }
    {
        VkCommandBufferAllocateInfo allocInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = m_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        vkAllocateCommandBuffers(m_device, &allocInfo, &m_handle);

        VkCommandBufferBeginInfo beginInfo { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                             .flags = oneTimeUse ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0u };

        vkBeginCommandBuffer(m_handle, &beginInfo);
    }

    ScopedCommandBuffer::~ScopedCommandBuffer()
    {
        vkEndCommandBuffer(m_handle);

        VkSubmitInfo submitInfo {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers    = &m_handle,
        };

        VkFence fence {};

        VkFenceCreateInfo fenceInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

        vkCreateFence(m_device, &fenceInfo, nullptr, &fence) >> vkResultChecker;

        vkQueueSubmit(m_queue, 1, &submitInfo, fence);
        vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);

        vkFreeCommandBuffers(m_device, m_pool, 1, &m_handle);

        vkDestroyFence(m_device, fence, nullptr);
    }

    CommandManager::CommandManager(Device const& device) : m_device { device }
    {
        VkCommandPoolCreateInfo graphicsPoolInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = device.getQueueFamilyIndices().graphicsFamily.value(),
        };

        VkCommandPoolCreateInfo transferPoolInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_device.getQueueFamilyIndices().transferFamily.value()
        };

        vkCreateCommandPool(device, &graphicsPoolInfo, nullptr, &m_graphicsCommandPool);
        vkCreateCommandPool(device, &transferPoolInfo, nullptr, &m_transferCommandPool);

        VkCommandBufferAllocateInfo allocInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = m_graphicsCommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = utils::size(m_graphicsCommandBuffers),
        };

        vkAllocateCommandBuffers(device, &allocInfo, m_graphicsCommandBuffers.data()) >> vkResultChecker;
    }

    CommandManager::~CommandManager()
    {
        vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
        vkDestroyCommandPool(m_device, m_transferCommandPool, nullptr);
    }
}  // namespace renderer::backend
