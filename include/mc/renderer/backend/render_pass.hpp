#pragma once

#include "device.hpp"
#include "surface.hpp"

namespace renderer::backend
{
    class RenderPass
    {
    public:
        RenderPass(Device const& device, Surface const& surface);

        RenderPass(RenderPass const&)                    = delete;
        RenderPass(RenderPass&&)                         = delete;
        auto operator=(RenderPass const&) -> RenderPass& = delete;
        auto operator=(RenderPass&&) -> RenderPass&      = delete;

        ~RenderPass();

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkRenderPass() const { return m_handle; }

    private:
        Device const& m_device;

        VkRenderPass m_handle { VK_NULL_HANDLE };
    };
}  // namespace renderer::backend
