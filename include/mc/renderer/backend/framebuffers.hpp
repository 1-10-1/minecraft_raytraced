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
        Framebuffers(Device const& device, RenderPass const& renderPass, Swapchain const& swapchain);
        ~Framebuffers();

        Framebuffers(Framebuffers const&) = delete;
        Framebuffers(Framebuffers&&)      = delete;

        auto operator=(Framebuffers const&) -> Framebuffers& = delete;
        auto operator=(Framebuffers&&) -> Framebuffers&      = delete;

        [[nodiscard]] auto operator[](size_t index) const -> VkFramebuffer { return m_swapChainFramebuffers[index]; }

    private:
        Device const& m_device;

        std::vector<VkFramebuffer> m_swapChainFramebuffers;
    };
}  // namespace renderer::backend
