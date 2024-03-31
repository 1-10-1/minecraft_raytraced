#pragma once

#include "device.hpp"
#include "surface.hpp"

#include <glm/ext/vector_uint2.hpp>

namespace renderer::backend
{
    class Swapchain
    {
    public:
        Swapchain(Device const& device, Surface& surface, glm::uvec2 initialDimensions);

        Swapchain(Swapchain const&)                    = delete;
        Swapchain(Swapchain&&)                         = delete;
        auto operator=(Swapchain const&) -> Swapchain& = delete;
        auto operator=(Swapchain&&) -> Swapchain&      = delete;

        ~Swapchain();

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkSwapchainKHR() const { return m_handle; }

    private:
        Device const& m_device;

        VkSwapchainKHR m_handle { VK_NULL_HANDLE };
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;
    };
}  // namespace renderer::backend
