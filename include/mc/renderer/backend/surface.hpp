#pragma once

#include "../../window.hpp"
#include "instance.hpp"

#include <vector>

#include <GLFW/glfw3.h>
#include <glm/ext/vector_uint2.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace renderer::backend
{
    struct SurfaceDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities {};
        std::vector<vk::SurfaceFormatKHR> formats {};
        std::vector<vk::PresentModeKHR> presentModes {};

        vk::Format format {};
        vk::Extent2D extent {};
        vk::SurfaceFormatKHR surfaceFormat {};
        vk::PresentModeKHR presentMode {};
    };

    class Surface
    {
    public:
        Surface()  = default;
        ~Surface() = default;

        Surface(window::Window& window, Instance& instance) : m_instance { &instance }, m_window { &window }
        {
            VkSurfaceKHR surface;

            glfwCreateWindowSurface(
                static_cast<vk::Instance>(instance), window.getHandle(), nullptr, &surface);

            m_surface = vk::raii::SurfaceKHR(instance.get(), surface);
        };

        Surface(Surface const&)                    = delete;
        auto operator=(Surface const&) -> Surface& = delete;

        auto operator=(Surface&&) -> Surface& = default;
        Surface(Surface&&)                    = default;

        [[nodiscard]] operator vk::SurfaceKHR() const { return m_surface; }

        [[nodiscard]] vk::SurfaceKHR const& get() const { return *m_surface; }

        [[nodiscard]] auto getDetails() const -> SurfaceDetails const& { return m_details; }

        [[nodiscard]] auto getFramebufferExtent() const -> vk::Extent2D { return m_details.extent; }

        [[nodiscard]] auto getVsync() const -> bool { return m_vsync; }

        void scheduleVsyncChange(bool vsync) { m_vsync = vsync; }

        void refresh(vk::PhysicalDevice device);

    private:
        Instance* m_instance { nullptr };
        window::Window* m_window { nullptr };
        vk::raii::SurfaceKHR m_surface { nullptr };
        SurfaceDetails m_details {};

        bool m_vsync { false };
    };
}  // namespace renderer::backend
