#include <mc/renderer/backend/surface.hpp>

#include <mc/exceptions.hpp>

namespace renderer::backend
{
    void Surface::refresh(VkPhysicalDevice device, VkExtent2D dimensions)
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &m_details.capabilities);

        uint32_t formatCount {};
        uint32_t presentModeCount {};

        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

        if (formatCount == 0 || presentModeCount == 0)
        {
            MC_THROW Error(GraphicsError, "Surface cannot be created for the given device");
        }

        m_details.formats.resize(formatCount);

        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, m_details.formats.data());

        m_details.presentModes.resize(presentModeCount);

        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, m_details.presentModes.data());

        // Choose extent
        {
            if (m_details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                m_details.extent = m_details.capabilities.currentExtent;
            }
            else
            {
                m_details.extent = { std::clamp(dimensions.width,
                                                m_details.capabilities.minImageExtent.width,
                                                m_details.capabilities.maxImageExtent.width),

                                     std::clamp(dimensions.height,
                                                m_details.capabilities.minImageExtent.height,
                                                m_details.capabilities.maxImageExtent.height) };
            }
        }

        // Choose format
        {
            m_details.surfaceFormat = m_details.formats[0];

            for (auto const& availableFormat : m_details.formats)
            {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    m_details.surfaceFormat = availableFormat;
                }
            }

            m_details.format = m_details.surfaceFormat.format;
        }

        // Choose present mode
        {
            bool presentModeChosen = false;

            if (!m_vsync)
            {
                for (auto const& availablePresentMode : m_details.presentModes)
                {
                    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                    {
                        m_details.presentMode = availablePresentMode;
                        presentModeChosen     = true;
                        break;
                    }

                    if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                    {
                        m_details.presentMode = availablePresentMode;
                        presentModeChosen     = true;
                    }
                }
            }

            if (!presentModeChosen)
            {
                m_details.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            }
        }
    }
}  // namespace renderer::backend
