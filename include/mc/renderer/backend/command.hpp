#pragma once

#include "device.hpp"

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class CommandManager
    {
    public:
        explicit CommandManager(Device const& device);
        ~CommandManager();

        CommandManager(CommandManager const&) = delete;
        CommandManager(CommandManager&&)      = delete;

        auto operator=(CommandManager const&) -> CommandManager& = delete;
        auto operator=(CommandManager&&) -> CommandManager&      = delete;

        [[nodiscard]] auto getCommandBuffer() const -> VkCommandBuffer { return m_cmdBuffer; }

    private:
        Device const& m_device;

        VkCommandPool m_cmdPool { VK_NULL_HANDLE };
        VkCommandBuffer m_cmdBuffer { VK_NULL_HANDLE };
    };
}  // namespace renderer::backend
