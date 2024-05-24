#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace renderer::backend
{
    class Instance
    {
    public:
        Instance();
        ~Instance() = default;

        Instance(Instance const&) = delete;
        Instance(Instance&&)      = default;

        auto operator=(Instance const&) -> Instance& = delete;
        auto operator=(Instance&&) -> Instance&      = default;

        [[nodiscard]] operator vk::Instance() const { return m_handle; }

        [[nodiscard]] auto get() const -> vk::raii::Instance const& { return m_handle; }

        [[nodiscard]] vk::raii::Instance const* operator->() const { return &m_handle; }

    private:
        void initValidationLayers(vk::raii::Context const& context);

        vk::raii::Instance m_handle { nullptr };
        vk::raii::DebugUtilsMessengerEXT m_debugMessenger { nullptr };
    };
}  // namespace renderer::backend
