#include <mc/renderer/backend/framebuffers.hpp>
#include <mc/renderer/backend/vk_checker.hpp>

#include <ranges>

#include <vulkan/vulkan.h>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace renderer::backend
{
    Framebuffers::Framebuffers(Device const& device, RenderPass const& renderPass, Swapchain const& swapchain)
        : m_device { device }, m_swapChainFramebuffers(swapchain.getImageViews().size())
    {
        VkExtent2D swapchainExtent = swapchain.getImageExtent();

        for (size_t i : vi::iota(0u, m_swapChainFramebuffers.size()))
        {
            std::array attachments = { swapchain.getImageViews()[i] };

            VkFramebufferCreateInfo framebufferInfo {
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = renderPass,
                .attachmentCount = 1,
                .pAttachments    = attachments.data(),
                .width           = swapchainExtent.width,
                .height          = swapchainExtent.height,
                .layers          = 1,
            };

            vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) >> vkResultChecker;
        }
    }

    Framebuffers::~Framebuffers()
    {
        for (auto* framebuffer : m_swapChainFramebuffers)
        {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
    }
}  // namespace renderer::backend
