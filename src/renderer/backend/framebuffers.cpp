#include <mc/renderer/backend/framebuffers.hpp>
#include <mc/renderer/backend/render_pass.hpp>
#include <mc/renderer/backend/swapchain.hpp>
#include <mc/renderer/backend/vk_checker.hpp>
#include <mc/utils.hpp>

#include <ranges>

#include <vulkan/vulkan.h>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace renderer::backend
{
    Framebuffers::Framebuffers(Device const& device,
                               RenderPass const& renderPass,
                               Swapchain const& swapchain,
                               VkImageView colorImageView,
                               VkImageView depthImageView)
        : m_device { device },
          m_swapChainFramebuffers(swapchain.getImageViews().size()),
          m_depthImageView { depthImageView }
    {
        create(renderPass, swapchain, colorImageView, depthImageView);
    }

    Framebuffers::~Framebuffers()
    {
        destroy();
    }

    void Framebuffers::create(RenderPass const& renderPass,
                              Swapchain const& swapchain,
                              VkImageView colorImageView,
                              VkImageView depthImageView)
    {
        VkExtent2D swapchainExtent = swapchain.getImageExtent();

        for (size_t i : vi::iota(0u, m_swapChainFramebuffers.size()))
        {
            std::array attachments = { colorImageView, depthImageView, swapchain.getImageViews()[i] };

            VkFramebufferCreateInfo framebufferInfo {
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = renderPass,
                .attachmentCount = utils::size(attachments),
                .pAttachments    = attachments.data(),
                .width           = swapchainExtent.width,
                .height          = swapchainExtent.height,
                .layers          = 1,
            };

            vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) >>
                vkResultChecker;
        }
    }

    void Framebuffers::destroy()
    {
        for (auto* framebuffer : m_swapChainFramebuffers)
        {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
    }
}  // namespace renderer::backend
