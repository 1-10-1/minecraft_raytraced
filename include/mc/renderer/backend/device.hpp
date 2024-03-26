#pragma once

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
            return graphicsFamily.has_value() && presentFamily.has_value() &&
                   transferFamily.has_value();
        };
    };

    class Device
    {
    public:
        explicit Device(VkInstance instance, Surface const& surface);
        ~Device();

        Device(Device const&) = delete;
        Device(Device&&)      = delete;

        auto operator=(Device const&) -> Device& = delete;
        auto operator=(Device&&) -> Device&      = delete;

        [[nodiscard]] auto get() const -> VkDevice { return m_device; }

        [[nodiscard]] auto getPhysical() const -> VkPhysicalDevice { return m_physicalDevice; }

    private:
        void selectPhysicalDevice(VkInstance instance, Surface const& surface);
        void selectLogicalDevice();

        static auto checkDeviceExtensionSupport(VkPhysicalDevice device) -> bool;

        static auto findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
            -> QueueFamilyIndices const&;

        VkDevice m_device { VK_NULL_HANDLE };
        VkPhysicalDevice m_physicalDevice { VK_NULL_HANDLE };

        QueueFamilyIndices m_queueFamilyIndices {};

        VkQueue m_graphicsQueue { VK_NULL_HANDLE };
        VkQueue m_presentQueue { VK_NULL_HANDLE };

        inline static std::vector<char const*> const m_requiredExtensions {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    };
}  // namespace renderer::backend
