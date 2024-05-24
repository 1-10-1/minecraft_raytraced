#pragma once

#include "device.hpp"
#include "surface.hpp"

#include <glm/ext/vector_uint2.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace renderer::backend
{
    class Swapchain
    {
    public:
        Swapchain();
        ~Swapchain() = default;

        Swapchain(Device const& device, Surface& surface, bool refreshSurface = true);

        Swapchain(Swapchain const&)                    = delete;
        auto operator=(Swapchain const&) -> Swapchain& = delete;

        Swapchain(Swapchain&& other) noexcept
            : m_device { std::exchange(other.m_device, nullptr) },
              m_handle { std::exchange(other.m_handle, nullptr) },
              m_images { std::exchange(other.m_images, {}) },
              m_imageViews { std::exchange(other.m_imageViews, {}) },
              m_imageExtent { std::exchange(other.m_imageExtent, {}) } {};

        auto operator=(Swapchain&& other) noexcept -> Swapchain&
        {
            if (this == &other)
            {
                return *this;
            }

            m_device      = std::exchange(other.m_device, nullptr);
            m_handle      = std::exchange(other.m_handle, nullptr);
            m_images      = std::exchange(other.m_images, {});
            m_imageViews  = std::exchange(other.m_imageViews, {});
            m_imageExtent = std::exchange(other.m_imageExtent, {});

            return *this;
        };

        [[nodiscard]] operator vk::raii::SwapchainKHR const&() const { return m_handle; }

        [[nodiscard]] operator vk::SwapchainKHR() const { return m_handle; }

        [[nodiscard]] auto operator->() const -> vk::raii::SwapchainKHR const* { return &m_handle; }

        [[nodiscard]] auto get() const -> vk::raii::SwapchainKHR const& { return m_handle; }

        [[nodiscard]] auto getImages() const -> std::vector<vk::Image> const& { return m_images; }

        [[nodiscard]] auto getImageViews() const -> std::vector<vk::raii::ImageView> const&
        {
            return m_imageViews;
        }

        [[nodiscard]] auto getImageExtent() const -> vk::Extent2D const& { return m_imageExtent; }

    private:
        Device const* m_device { nullptr };

        vk::raii::SwapchainKHR m_handle { nullptr };

        std::vector<vk::Image> m_images {};
        std::vector<vk::raii::ImageView> m_imageViews {};

        vk::Extent2D m_imageExtent {};
    };
}  // namespace renderer::backend
