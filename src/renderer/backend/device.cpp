#include <mc/exceptions.hpp>
#include <mc/renderer/backend/device.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <unordered_set>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    struct DeviceCandidate
    {
        VkPhysicalDevice device {};
        VkPhysicalDeviceProperties properties {};
        VkPhysicalDeviceFeatures features {};
        QueueFamilyIndices queueFamilyIndices {};
        int score {};
    };

    Device::Device(Instance& instance, Surface const& surface)
    {
        selectPhysicalDevice(instance, surface);
        selectLogicalDevice();
    }

    void Device::selectPhysicalDevice(Instance& instance, Surface const& surface)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            MC_THROW Error(GraphicsError, "Failed to find GPUs with vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);

        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        DeviceCandidate bestCandidate { .score = 0 };

        for (auto* device : devices)
        {
            auto const& queueFamilyIndices { findQueueFamilies(device, surface) };

            VkPhysicalDeviceProperties deviceProperties {};
            VkPhysicalDeviceFeatures deviceFeatures {};

            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            // clang-format off
            std::array necessaryConditions { std::to_array<std::pair<std::string_view, bool>>({
                { "Geometry shader availability",
                  static_cast<bool>(deviceFeatures.geometryShader)    },

                { "Anisotropy availability",
                  static_cast<bool>(deviceFeatures.samplerAnisotropy) },

                { "Necessary queues present",
                  queueFamilyIndices.isComplete()                     },

                { "Necessary extensions supported",
                  checkDeviceExtensionSupport(device)                 }
            })};
            // clang-format on

            bool necessaryConditionsMet = true;

            for (auto [conditionName, condition] : necessaryConditions)
            {
                if (!condition)
                {
                    necessaryConditionsMet = false;

                    logger::trace("Graphics card {} rejected as it was unable to satisfy the "
                                  "following condition: {}",
                                  deviceProperties.deviceName,
                                  conditionName);

                    break;
                }
            }

            if (!necessaryConditionsMet)
            {
                continue;
            }

            int score = 0;

            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                score += 1000;
            }

            score += static_cast<int>(deviceProperties.limits.maxImageDimension2D);

            if (score > bestCandidate.score)
            {
                bestCandidate = { .device             = device,
                                  .properties         = deviceProperties,
                                  .features           = deviceFeatures,
                                  .queueFamilyIndices = queueFamilyIndices,
                                  .score              = score };
            }
        }

        if (bestCandidate.score == 0)
        {
            MC_THROW Error(GraphicsError, "Could not find a suitable graphics card");
        }

        m_physicalHandle     = bestCandidate.device;
        m_queueFamilyIndices = bestCandidate.queueFamilyIndices;

        std::string_view deviceType;

        switch (bestCandidate.properties.deviceType)
        {
            case (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU):
                deviceType = "Integrated";
                break;
            case (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU):
                deviceType = "Discrete GPU";
                break;
            case (VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU):
                deviceType = "Virtual GPU";
                break;
            case (VK_PHYSICAL_DEVICE_TYPE_CPU):
                deviceType = "CPU";
                break;
            default:
                deviceType = "Unknown";
        }

        logger::info(R"(Using physical device "{}" of type "{}")", bestCandidate.properties.deviceName, deviceType);
    }

    void Device::selectLogicalDevice()
    {
        std::unordered_set<uint32_t> queueFamilies = { m_queueFamilyIndices.graphicsFamily.value(),
                                                       m_queueFamilyIndices.presentFamily.value(),
                                                       m_queueFamilyIndices.transferFamily.value() };

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        float queuePriority = 1.0f;

        queueCreateInfos.reserve(queueFamilies.size());

        for (uint32_t queueFamily : queueFamilies)
        {
            queueCreateInfos.push_back({ .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                         .queueFamilyIndex = queueFamily,
                                         .queueCount       = 1,
                                         .pQueuePriorities = &queuePriority });
        }

        VkPhysicalDeviceFeatures deviceFeatures {};

        VkDeviceCreateInfo deviceCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount    = Utils::size(queueCreateInfos),
            .pQueueCreateInfos       = queueCreateInfos.data(),
            .enabledLayerCount       = 0,
            .ppEnabledLayerNames     = nullptr,
            .enabledExtensionCount   = Utils::size(m_requiredExtensions),
            .ppEnabledExtensionNames = m_requiredExtensions.data(),
            .pEnabledFeatures        = &deviceFeatures,
        };

        vkCreateDevice(m_physicalHandle, &deviceCreateInfo, nullptr, &m_logicalHandle) >> vkResultChecker;

        vkGetDeviceQueue(m_logicalHandle, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);

        vkGetDeviceQueue(m_logicalHandle, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
    }

    auto Device::checkDeviceExtensionSupport(VkPhysicalDevice device) -> bool
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::unordered_set<std::string> requiredExtensions(m_requiredExtensions.begin(), m_requiredExtensions.end());

        for (auto const& extension : availableExtensions)
        {
            requiredExtensions.erase(static_cast<char const*>(extension.extensionName));
        }

        return requiredExtensions.empty();
    }

    auto Device::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) -> QueueFamilyIndices const&
    {
        static std::unordered_map<VkPhysicalDevice, QueueFamilyIndices> memo;

        if (memo.find(device) != memo.end())
        {
            return memo[device];
        }

        auto& indices = memo[device];

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; auto const& queueFamily : queueFamilies)
        {
            if (indices.isComplete())
            {
                break;
            }

            if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0u && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0u)
            {
                indices.transferFamily = i;
            }

            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = 0u;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport != 0u)
            {
                indices.presentFamily = i;
            }

            ++i;
        }

        return indices;
    }

    Device::~Device()
    {
        vkDestroyDevice(m_logicalHandle, nullptr);
    }
}  // namespace renderer::backend
