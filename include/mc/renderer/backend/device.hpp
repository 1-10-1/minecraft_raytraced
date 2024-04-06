#pragma once

#include "instance.hpp"

#include <optional>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class Surface;

    class QueueFamilyIndices
    {
    public:
        QueueFamilyIndices() = default;

        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> transferFamily;

        [[nodiscard]] auto isComplete() const -> bool
        {
            return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
        };
    };

    class Device
    {
    public:
        explicit Device(Instance& instance, Surface const& surface);
        ~Device();

        Device(Device const&) = delete;
        Device(Device&&)      = default;

        auto operator=(Device const&) -> Device& = delete;
        auto operator=(Device&&) -> Device&      = default;

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkDevice() const { return m_logicalHandle; }

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkPhysicalDevice() const { return m_physicalHandle; }

        [[nodiscard]] auto getQueueFamilyIndices() const -> QueueFamilyIndices const& { return m_queueFamilyIndices; }

        [[nodiscard]] auto getGraphicsQueue() const -> VkQueue { return m_graphicsQueue; }

        [[nodiscard]] auto getPresentQueue() const -> VkQueue { return m_presentQueue; }

    private:
        void selectPhysicalDevice(Instance& instance, Surface const& surface);
        void selectLogicalDevice();

        static auto checkDeviceExtensionSupport(VkPhysicalDevice device) -> bool;

        static auto findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) -> QueueFamilyIndices const&;

        VkDevice m_logicalHandle { VK_NULL_HANDLE };
        VkPhysicalDevice m_physicalHandle { VK_NULL_HANDLE };

        QueueFamilyIndices m_queueFamilyIndices {};

        VkQueue m_graphicsQueue { VK_NULL_HANDLE };
        VkQueue m_presentQueue { VK_NULL_HANDLE };

        // clang-format off
        inline static std::vector<char const*> const m_requiredExtensions
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if PROFILED
            VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
#endif
        };
        // clang-format on
    };
}  // namespace renderer::backend
