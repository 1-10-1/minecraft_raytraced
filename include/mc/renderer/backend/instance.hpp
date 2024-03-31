#pragma once

#include <array>

#include <vulkan/vulkan.h>

namespace renderer::backend
{
    class Instance
    {
    public:
        Instance();
        ~Instance();

        Instance(Instance const&) = delete;
        Instance(Instance&&)      = delete;

        auto operator=(Instance const&) -> Instance& = delete;
        auto operator=(Instance&&) -> Instance&      = delete;

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkInstance() const { return m_handle; }

    private:
        void initValidationLayers();

        static VKAPI_ATTR auto VKAPI_CALL validationLayerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                  VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                                                  void* pUserData) -> VkBool32;

        VkInstance m_handle { VK_NULL_HANDLE };

        VkDebugUtilsMessengerEXT m_debugMessenger { nullptr };

        static constexpr VkDebugUtilsMessengerCreateInfoEXT m_debugMessengerInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

            .pfnUserCallback = validationLayerCallback,
        };

#if DEBUG
        static constexpr std::array m_validationLayers { "VK_LAYER_KHRONOS_validation" };
#else
        static constexpr std::array<char const*, 0> m_validationLayers {};
#endif
    };
}  // namespace renderer::backend
