#include "mc/defines.hpp"
#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/instance.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <print>
#include <ranges>
#include <stacktrace>
#include <vector>

#include <GLFW/glfw3.h>
#include <fmt/core.h>
#include <tracy/Tracy.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace renderer::backend
{
    Instance::Instance()
    {
        VkApplicationInfo appInfo {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName   = "Untitled",
            .applicationVersion = VK_VERSION_1_0,
            .pEngineName        = "No engine",
            .engineVersion      = VK_VERSION_1_0,
            .apiVersion         = VK_API_VERSION_1_3,
        };

        std::vector<char const*> requiredExtensions;
        std::vector<VkExtensionProperties> supportedExtensions;

        {
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

            supportedExtensions.resize(extensionCount);

            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data());
        }

        {
            uint32_t count {};
            char const** glfwExtStrings = glfwGetRequiredInstanceExtensions(&count);

            requiredExtensions.reserve(count);

            for (uint32_t i = 0; i < count; ++i)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                requiredExtensions.push_back(glfwExtStrings[i]);
            }
        }

        if constexpr (kDebug)
        {
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        for (char const* requiredExt : requiredExtensions)
        {
            if (auto it = rn::find_if(supportedExtensions,
                                      [requiredExt](VkExtensionProperties const& supportedExt)
                                      {
                                          return std::string_view(static_cast<char const*>(
                                                     supportedExt.extensionName)) == requiredExt;
                                      });
                it == supportedExtensions.end())
            {
                MC_THROW Error(GraphicsError, std::format("Extension {} is required but isn't supported", requiredExt));
            }
        }

        VkInstanceCreateInfo instanceInfo {
            .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                   = kDebug ? &m_debugMessengerInfo : nullptr,
            .pApplicationInfo        = &appInfo,
            .enabledLayerCount       = Utils::size(m_validationLayers),
            .ppEnabledLayerNames     = m_validationLayers.data(),
            .enabledExtensionCount   = Utils::size(requiredExtensions),
            .ppEnabledExtensionNames = requiredExtensions.data(),
        };

        vkCreateInstance(&instanceInfo, nullptr, &m_handle) >> vkResultChecker;

        if constexpr (kDebug)
        {
            initValidationLayers();
        }
    }

    Instance::~Instance()
    {
        if constexpr (kDebug)
        {
            std::function destroyDebugMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(m_handle, "vkDestroyDebugUtilsMessengerEXT"));

            destroyDebugMessenger(m_handle, m_debugMessenger, nullptr);
        }

        vkDestroyInstance(m_handle, nullptr);
    }

    void Instance::initValidationLayers()
    {
        uint32_t layerCount {};
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (char const* neededLayer : m_validationLayers)
        {
            if (auto it = rn::find_if(availableLayers,
                                      [neededLayer](VkLayerProperties const& layer)
                                      {
                                          return std::string_view(static_cast<char const*>(layer.layerName)) ==
                                                 neededLayer;
                                      });
                it == availableLayers.end())
            {
                logger::warn("Validation layer '{}' was requested but isn't available", neededLayer);
            };
        }

        std::function createDebugMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_handle, "vkCreateDebugUtilsMessengerEXT"));

        createDebugMessenger(m_handle, &m_debugMessengerInfo, nullptr, &m_debugMessenger) >> vkResultChecker;
    };

    VKAPI_ATTR auto VKAPI_CALL
    Instance::validationLayerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                      VkDebugUtilsMessageTypeFlagsEXT messageType,
                                      VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                      void* pUserData) -> VkBool32
    {
        ZoneScopedN("Validation layer callback");

        std::string_view message = pCallbackData->pMessage;

        if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
            message.ends_with("uses API version 1.2 which is older than the application "
                              "specified API version of "
                              "1.3. May cause issues."))
        {
            return VK_FALSE;
        }

        // [[maybe_unused]] Renderer* renderer { static_cast<Renderer*>(pUserData) };

        std::string type;

        switch (messageType)
        {
            case (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT):
                type = "General";
                break;
            case (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT):
                type = "Validation";
                break;
            case (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT):
                type = "Performance";
                break;
            default:
                type = "Unknown";
        }

        auto stacktrace = std::stacktrace::current();
        std::string srcFile;
        std::string srcFunc;
        int srcLine {};

        for (auto [i, trace] : vi::enumerate(stacktrace))
        {
            // Find the second source file on the stacktrace thats inside the root source path
            // (the first stacktrace is the one we're in right now (this function))
            if (trace.source_file().starts_with(ROOT_SOURCE_PATH) && i > 1)
            {
                srcFile = trace.source_file();
                srcFunc = trace.description();
                srcLine = static_cast<int>(trace.source_line());
                break;
            };
        }

        spdlog::source_loc location(srcFile.data(), srcLine, srcFunc.data());

        switch (messageSeverity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                logger::logAt<logger::level::warn>(location, "({}) {}", type, pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                logger::logAt<logger::level::err>(location, "({}) {}", type, pCallbackData->pMessage);
                break;
            default:
                return VK_FALSE;
        }

        return VK_FALSE;
    }
}  // namespace renderer::backend
