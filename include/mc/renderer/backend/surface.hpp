#pragma once

#include "../../window.hpp"
#include "instance.hpp"

#include <vector>

#include <GLFW/glfw3.h>
#include <glm/ext/vector_uint2.hpp>
#include <vulkan/vulkan.h>

namespace renderer::backend
{
    struct SurfaceDetails
    {
        VkSurfaceCapabilitiesKHR capabilities {};
        std::vector<VkSurfaceFormatKHR> formats {};
        std::vector<VkPresentModeKHR> presentModes {};

        VkFormat format {};
        VkExtent2D extent {};
        VkSurfaceFormatKHR surfaceFormat {};
        VkPresentModeKHR presentMode {};
    };

    class Surface
    {
    public:
        Surface(window::Window& window, Instance& instance) : m_instance { instance }, m_window { window }
        {
            glfwCreateWindowSurface(instance, window.getHandle(), nullptr, &m_surface);
        };

        ~Surface() { vkDestroySurfaceKHR(m_instance, m_surface, nullptr); };

        Surface(Surface&&)      = delete;
        Surface(Surface const&) = delete;

        auto operator=(Surface const&) -> Surface& = delete;
        auto operator=(Surface&&) -> Surface&      = delete;

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkSurfaceKHR() const { return m_surface; }

        [[nodiscard]] auto getDetails() const -> SurfaceDetails const& { return m_details; }

        [[nodiscard]] auto getFramebufferExtent() const -> VkExtent2D { return m_details.extent; }

        void refresh(VkPhysicalDevice device);

    private:
        Instance& m_instance;
        window::Window& m_window;
        VkSurfaceKHR m_surface { VK_NULL_HANDLE };
        SurfaceDetails m_details {};

        bool m_vsync { false };
    };
}  // namespace renderer::backend
