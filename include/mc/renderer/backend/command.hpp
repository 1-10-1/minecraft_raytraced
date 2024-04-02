#pragma once

#include "constants.hpp"
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

        [[nodiscard]] auto getCommandBuffer(size_t frameIndex) const -> VkCommandBuffer { return m_cmdBuffers[frameIndex]; }

    private:
        Device const& m_device;

        VkCommandPool m_cmdPool { VK_NULL_HANDLE };

        std::array<VkCommandBuffer, kNumFramesInFlight> m_cmdBuffers {};
    };
}  // namespace renderer::backend
