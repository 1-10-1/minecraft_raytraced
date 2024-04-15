#pragma once

#include "instance.hpp"

#include <optional>

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

        [[nodiscard]] auto getTransferQueue() const -> VkQueue { return m_transferQueue; }

        [[nodiscard]] auto getPresentQueue() const -> VkQueue { return m_presentQueue; }

        [[nodiscard]] auto getDeviceProperties() const -> VkPhysicalDeviceProperties const&
        {
            return m_deviceProperties;
        }

        [[nodiscard]] auto getDeviceFeatures() const -> VkPhysicalDeviceFeatures const& { return m_deviceFeatures; }

        [[nodiscard]] auto getFormatProperties(VkFormat format) const -> VkFormatProperties
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physicalHandle, format, &props);

            return props;
        }

        [[nodiscard]] auto findSuitableMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
            -> uint32_t;

    private:
        void selectPhysicalDevice(Instance& instance, Surface const& surface);
        void selectLogicalDevice();

        VkDevice m_logicalHandle { VK_NULL_HANDLE };
        VkPhysicalDevice m_physicalHandle { VK_NULL_HANDLE };

        VkPhysicalDeviceProperties m_deviceProperties {};
        VkPhysicalDeviceFeatures m_deviceFeatures {};
        VkPhysicalDeviceMemoryProperties m_memoryProperties {};
        VkSampleCountFlagBits m_sampleCount {};

        QueueFamilyIndices m_queueFamilyIndices {};

        VkQueue m_graphicsQueue { VK_NULL_HANDLE };
        VkQueue m_presentQueue { VK_NULL_HANDLE };
        VkQueue m_transferQueue { VK_NULL_HANDLE };

    };  // namespace renderer::backend
}  // namespace renderer::backend
