#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    CommandManager::CommandManager(Device const& device) : m_device { device }
    {
        VkCommandPoolCreateInfo poolInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = device.getQueueFamilyIndices().graphicsFamily.value(),
        };

        vkCreateCommandPool(device, &poolInfo, nullptr, &m_cmdPool);

        VkCommandBufferAllocateInfo allocInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = m_cmdPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        vkAllocateCommandBuffers(device, &allocInfo, &m_cmdBuffer) >> vkResultChecker;
    }

    CommandManager::~CommandManager()
    {
        vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
    }
}  // namespace renderer::backend
