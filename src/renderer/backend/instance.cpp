#include <mc/asserts.hpp>
#include <mc/defines.hpp>
#include <mc/exceptions.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/backend/instance.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <algorithm>
#include <array>
#include <expected>
#include <print>
#include <ranges>
#include <stacktrace>
#include <vector>

#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace
{
    using namespace renderer::backend;

    VKAPI_ATTR auto VKAPI_CALL
    validationLayerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                            VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                            void* pUserData) -> VkBool32;

    vk::DebugUtilsMessengerCreateInfoEXT constexpr debugMessengerInfo {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,

        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding,

        .pfnUserCallback = validationLayerCallback
    };

#if DEBUG
    std::array const m_validationLayers { "VK_LAYER_KHRONOS_validation" };
#else
    std::array<char const*, 0> const m_validationLayers {};
#endif
}  // namespace

namespace renderer::backend
{
    Instance::Instance()
    {
        vk::raii::Context context {};

        MC_ASSERT(context.enumerateInstanceVersion() >= vk::ApiVersion13);

        vk::ApplicationInfo applicationInfo { .pApplicationName   = "Minecraft",
                                              .applicationVersion = 1,
                                              .pEngineName        = "Untitled",
                                              .engineVersion      = 1,
                                              .apiVersion         = vk::ApiVersion13 };

        std::vector<char const*> requiredExtensions;
        std::vector<vk::ExtensionProperties> supportedExtensions =
            context.enumerateInstanceExtensionProperties();

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
                MC_THROW Error(GraphicsError,
                               std::format("Extension {} is required but isn't supported", requiredExt));
            }
        }

        m_handle = context.createInstance(
                       vk::InstanceCreateInfo { .pApplicationInfo        = &applicationInfo,
                                                .enabledLayerCount       = utils::size(m_validationLayers),
                                                .ppEnabledLayerNames     = m_validationLayers.data(),
                                                .enabledExtensionCount   = utils::size(requiredExtensions),
                                                .ppEnabledExtensionNames = requiredExtensions.data() }) >>
                   ResultChecker();

        if constexpr (kDebug)
        {
            initValidationLayers(context);
        }
    }

    void Instance::initValidationLayers(vk::raii::Context const& context)
    {
        std::vector<vk::LayerProperties> availableLayers = context.enumerateInstanceLayerProperties();

        for (char const* neededLayer : m_validationLayers)
        {
            if (auto it = rn::find_if(availableLayers,
                                      [neededLayer](VkLayerProperties const& layer)
                                      {
                                          return std::string_view(static_cast<char const*>(
                                                     layer.layerName)) == neededLayer;
                                      });
                it == availableLayers.end())
            {
                logger::warn("Validation layer '{}' was requested but isn't available", neededLayer);
            };
        }

        m_debugMessenger = m_handle.createDebugUtilsMessengerEXT(debugMessengerInfo) >> ResultChecker();
    };

}  // namespace renderer::backend

namespace
{
    using namespace renderer::backend;

    VKAPI_ATTR auto VKAPI_CALL
    validationLayerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                            VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                            void* pUserData) -> VkBool32
    {
        ZoneScopedN("Validation layer callback");

        std::string_view message = pCallbackData->pMessage;

        if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
            message.ends_with("not consumed by vertex shader."))  // TODO(aether)
        {
            return VK_FALSE;
        }

        // [[maybe_unused]] RendererBackend* renderer { static_cast<RendererBackend*>(pUserData) };

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
            if (i > 1 && trace.source_file().starts_with(ROOT_SOURCE_PATH))
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
}  // namespace
