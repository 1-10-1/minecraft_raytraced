#include <mc/renderer/backend/swapchain.hpp>
#include <mc/renderer/backend/vk_checker.hpp>

#include <vulkan/vulkan_core.h>

#include <ranges>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

namespace renderer::backend
{
    Swapchain::Swapchain(Device const& device, Surface& surface) : m_device { device }
    {
        create(surface, false);
    }

    Swapchain::~Swapchain()
    {
        destroy();
    }

    void Swapchain::create(Surface& surface, bool refreshSurface)
    {
        if (refreshSurface)
        {
            surface.refresh(m_device);
        }

        auto const& details = surface.getDetails();

        m_imageExtent = details.extent;

        std::uint32_t imageCount = details.capabilities.minImageCount + 1;

        if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
        {
            imageCount = details.capabilities.maxImageCount;
        }

        QueueFamilyIndices const& queueFamilyIndices = m_device.getQueueFamilyIndices();

        std::array<std::uint32_t, 2> queueFamilyIndicesArray { queueFamilyIndices.graphicsFamily.value(),
                                                               queueFamilyIndices.presentFamily.value() };

        bool sameQueueFamily = queueFamilyIndices.graphicsFamily == queueFamilyIndices.presentFamily;

        VkSwapchainCreateInfoKHR createInfo {
            .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface               = surface,
            .minImageCount         = imageCount,
            .imageFormat           = details.surfaceFormat.format,
            .imageColorSpace       = details.surfaceFormat.colorSpace,
            .imageExtent           = details.extent,
            .imageArrayLayers      = 1,
            .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode      = sameQueueFamily ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = sameQueueFamily ? 0u : 2u,
            .pQueueFamilyIndices   = sameQueueFamily ? nullptr : queueFamilyIndicesArray.data(),
            .preTransform          = details.capabilities.currentTransform,
            .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode           = details.presentMode,
            .clipped               = VK_TRUE,
            .oldSwapchain          = VK_NULL_HANDLE,
        };

        vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_handle) >> vkResultChecker;

        vkGetSwapchainImagesKHR(m_device, m_handle, &imageCount, nullptr);

        m_images.resize(imageCount);
        m_imageViews.resize(imageCount);

        vkGetSwapchainImagesKHR(m_device, m_handle, &imageCount, m_images.data());

        for (size_t i : vi::iota(0u, imageCount))
        {
            VkImageViewCreateInfo createInfo {
                .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image            = m_images[i],
                .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                .format           = details.format,

                .components       = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                      .a = VK_COMPONENT_SWIZZLE_IDENTITY, },

                .subresourceRange = { .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                      .baseMipLevel   = 0,
                                      .levelCount     = 1,
                                      .baseArrayLayer = 0,
                                      .layerCount     = 1, },
            };

            vkCreateImageView(m_device, &createInfo, nullptr, &m_imageViews[i]);
        }
    }

    void Swapchain::destroy()
    {
        for (auto* imageView : m_imageViews)
        {
            vkDestroyImageView(m_device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_device, m_handle, nullptr);
    }
}  // namespace renderer::backend
