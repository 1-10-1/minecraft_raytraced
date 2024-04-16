#pragma once

#include "device.hpp"
#include "render_pass.hpp"
#include "swapchain.hpp"

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class Framebuffers
    {
    public:
        explicit Framebuffers(Device const& device,
                              RenderPass const& renderPass,
                              Swapchain const& swapchain,
                              VkImageView colorImageView,
                              VkImageView depthImageView);
        ~Framebuffers();

        Framebuffers(Framebuffers const&) = delete;
        Framebuffers(Framebuffers&&)      = delete;

        auto operator=(Framebuffers const&) -> Framebuffers& = delete;
        auto operator=(Framebuffers&&) -> Framebuffers&      = delete;

        [[nodiscard]] auto operator[](size_t index) const -> VkFramebuffer { return m_swapChainFramebuffers[index]; }

        void create(RenderPass const& renderPass,
                    Swapchain const& swapchain,
                    VkImageView colorImageView,
                    VkImageView depthImageView);
        void destroy();

    private:
        Device const& m_device;

        std::vector<VkFramebuffer> m_swapChainFramebuffers;
        VkImageView m_depthImageView;
    };
}  // namespace renderer::backend
