#include "mc/renderer/backend/constants.hpp"
#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/device.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <unordered_set>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace
{
    using namespace renderer::backend;

    // clang-format off
    constexpr std::array requiredExtensions
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
#if PROFILED
        VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
#endif
    };

    // clang-format on

    auto checkDeviceExtensionSupport(VkPhysicalDevice device) -> bool
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::unordered_set<std::string> requiredExtensionsSet(requiredExtensions.begin(), requiredExtensions.end());

        for (auto const& extension : availableExtensions)
        {
            requiredExtensionsSet.erase(static_cast<char const*>(extension.extensionName));
        }

        return requiredExtensionsSet.empty();
    }

    auto findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) -> QueueFamilyIndices const&
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

            if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0u &&
                (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0u)
            {
                logger::debug("Found a dedicated transfer queue!");
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

        if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value())
        {
            logger::debug("Could not find a dedicated transfer queue. Using the graphics queue for this purpose.");
            indices.transferFamily = indices.graphicsFamily.value();
        }

        return indices;
    }
}  // namespace

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

    Device::Device(Instance& instance, Surface& surface)
    {
        selectPhysicalDevice(instance, surface);
        selectLogicalDevice();
    }

    void Device::selectPhysicalDevice(Instance& instance, Surface& surface)
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

        VkSampleCountFlags sampleCounts = bestCandidate.properties.limits.framebufferColorSampleCounts &
                                          bestCandidate.properties.limits.framebufferDepthSampleCounts;

        std::array possibleSampleCounts {
            VK_SAMPLE_COUNT_2_BIT,  VK_SAMPLE_COUNT_4_BIT,  VK_SAMPLE_COUNT_8_BIT,
            VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_64_BIT,
        };

        m_sampleCount = VK_SAMPLE_COUNT_1_BIT;

        for (auto count : possibleSampleCounts)
        {
            if ((count & sampleCounts) != 0)
            {
                m_sampleCount = count > kMaxSamples ? kMaxSamples : count;
            }
        }

        logger::info("Using msaa x{}", std::to_underlying(m_sampleCount));

        logger::debug("Work group size: {}x{}x{} | Work group invocations: {} | Work group count: {}x{}x{}",
                      bestCandidate.properties.limits.maxComputeWorkGroupSize[0],
                      bestCandidate.properties.limits.maxComputeWorkGroupSize[1],
                      bestCandidate.properties.limits.maxComputeWorkGroupSize[2],
                      bestCandidate.properties.limits.maxComputeWorkGroupInvocations,
                      bestCandidate.properties.limits.maxComputeWorkGroupCount[0],
                      bestCandidate.properties.limits.maxComputeWorkGroupCount[1],
                      bestCandidate.properties.limits.maxComputeWorkGroupCount[2]);

        std::string_view deviceType;

        surface.refresh(m_physicalHandle);

        switch (bestCandidate.properties.deviceType)
        {
            case (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU):
                deviceType = "integrated gpu";
                break;
            case (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU):
                deviceType = "discrete GPU";
                break;
            case (VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU):
                deviceType = "virtual GPU";
                break;
            case (VK_PHYSICAL_DEVICE_TYPE_CPU):
                deviceType = "CPU";
                break;
        }

        logger::info("Using {} {} for rendering", deviceType, bestCandidate.properties.deviceName);
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

        VkPhysicalDeviceVulkan12Features features12 {
            .sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .descriptorIndexing  = VK_TRUE,
            .bufferDeviceAddress = VK_TRUE,
        };

        VkPhysicalDeviceVulkan13Features features13 {
            .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext            = &features12,
            .synchronization2 = VK_TRUE,
            .dynamicRendering = VK_TRUE,
        };

        VkPhysicalDeviceFeatures2 deviceFeatures {
            .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext    = &features13,
            .features = { .fillModeNonSolid              = VK_TRUE,
                          .samplerAnisotropy             = VK_TRUE,
                          .shaderStorageImageMultisample = VK_TRUE, },
        };

        VkDeviceCreateInfo deviceCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                   = &deviceFeatures,
            .queueCreateInfoCount    = utils::size(queueCreateInfos),
            .pQueueCreateInfos       = queueCreateInfos.data(),
            .enabledLayerCount       = 0,
            .ppEnabledLayerNames     = nullptr,
            .enabledExtensionCount   = utils::size(requiredExtensions),
            .ppEnabledExtensionNames = requiredExtensions.data(),
        };

        vkCreateDevice(m_physicalHandle, &deviceCreateInfo, nullptr, &m_logicalHandle) >> vkResultChecker;

        vkGetDeviceQueue(m_logicalHandle, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);

        vkGetDeviceQueue(m_logicalHandle, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);

        vkGetDeviceQueue(m_logicalHandle, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue);
    }

    auto Device::findSuitableMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const -> uint32_t
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalHandle, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) != 0 &&
                (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        MC_THROW Error(RendererError, "Failed to find a suitable memory type.");
    }

    Device::~Device()
    {
        vkDestroyDevice(m_logicalHandle, nullptr);
    }
}  // namespace renderer::backend
