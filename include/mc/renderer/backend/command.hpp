#pragma once

#include "constants.hpp"
#include "device.hpp"

#include <array>

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class ScopedCommandBuffer
    {
    public:
        ScopedCommandBuffer(Device& device, VkCommandPool commandPool, VkQueue queue, bool oneTimeUse = false);
        ~ScopedCommandBuffer();

        ScopedCommandBuffer(ScopedCommandBuffer const&)                    = delete;
        ScopedCommandBuffer(ScopedCommandBuffer&&)                         = delete;
        auto operator=(ScopedCommandBuffer const&) -> ScopedCommandBuffer& = delete;
        auto operator=(ScopedCommandBuffer&&) -> ScopedCommandBuffer&      = delete;

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkCommandBuffer() const { return m_handle; }

    private:
        Device& m_device;

        VkCommandPool m_pool;
        VkQueue m_queue;

        VkCommandBuffer m_handle { VK_NULL_HANDLE };
    };

    class CommandManager
    {
    public:
        explicit CommandManager(Device const& device);
        ~CommandManager();

        CommandManager(CommandManager const&) = delete;
        CommandManager(CommandManager&&)      = delete;

        auto operator=(CommandManager const&) -> CommandManager& = delete;
        auto operator=(CommandManager&&) -> CommandManager&      = delete;

        [[nodiscard]] auto getGraphicsCmdBuffer(size_t index) const -> VkCommandBuffer
        {
            return m_graphicsCommandBuffers[index];
        }

        [[nodiscard]] auto getGraphicsCmdPool() const -> VkCommandPool { return m_graphicsCommandPool; }

        [[nodiscard]] auto getTransferCmdPool() const -> VkCommandPool { return m_transferCommandPool; }

    private:
        Device const& m_device;

        VkCommandPool m_graphicsCommandPool {};
        VkCommandPool m_transferCommandPool {};

        std::array<VkCommandBuffer, kNumFramesInFlight> m_graphicsCommandBuffers {};
    };
}  // namespace renderer::backend
