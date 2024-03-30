#pragma once

#include "../../events.hpp"
#include "device.hpp"
#include "instance.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

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
        Surface(GLFWwindow* window, VkInstance instance) : m_instance { instance }
        {
            glfwCreateWindowSurface(instance, window, nullptr, &m_surface);
        };

        ~Surface() { vkDestroySurfaceKHR(m_instance, m_surface, nullptr); };

        Surface(Surface&&)      = delete;
        Surface(Surface const&) = delete;

        auto operator=(Surface const&) -> Surface& = delete;
        auto operator=(Surface&&) -> Surface&      = delete;

        [[nodiscard]] auto get() const -> VkSurfaceKHR { return m_surface; }

        void refresh(VkPhysicalDevice device, VkExtent2D dimensions);

    private:
        VkInstance m_instance { VK_NULL_HANDLE };
        VkSurfaceKHR m_surface { VK_NULL_HANDLE };
        SurfaceDetails m_details {};

        bool m_vsync { true };
    };

    class RendererBackend
    {
    public:
        explicit RendererBackend(GLFWwindow* window, glm::uvec2 initialFramebufferDimensions);

        void render();

        void onFramebufferResized(WindowFramebufferResizeEvent const& event)
        {
            m_surface.refresh(m_device.getPhysical(),
                              { .width = event.dimensions.x, .height = event.dimensions.y });
        };

    private:
        Instance m_instance;
        GLFWwindow* m_window;
        Surface m_surface;
        Device m_device;
    };
}  // namespace renderer::backend
