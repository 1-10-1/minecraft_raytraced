#pragma once

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

        VkInstance m_handle { VK_NULL_HANDLE };
        VkDebugUtilsMessengerEXT m_debugMessenger { nullptr };
    };
}  // namespace renderer::backend
