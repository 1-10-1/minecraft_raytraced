#include <mc/renderer/backend/renderer_backend.hpp>

#include <algorithm>
#include <limits>

namespace renderer::backend
{
    RendererBackend::RendererBackend(GLFWwindow* window)
        : m_window { window },
          m_surface { window, m_instance.get() },
          m_device { Device(m_instance.get(), m_surface) }
    {
    }

    void Surface::refresh(VkPhysicalDevice device, VkExtent2D dimensions)
    {
        SurfaceDetails details {};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

        uint32_t formatCount {};

        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);

            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device, m_surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount {};
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);

            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, m_surface, &presentModeCount, details.presentModes.data());
        }

        // Choose extent
        {
            if (details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                details.extent = { details.capabilities.currentExtent.width,
                                   details.capabilities.currentExtent.height };
            }

            details.extent = { std::clamp(dimensions.width,
                                          details.capabilities.minImageExtent.width,
                                          details.capabilities.maxImageExtent.width),

                               std::clamp(dimensions.height,
                                          details.capabilities.minImageExtent.height,
                                          details.capabilities.maxImageExtent.height) };
        }

        // Choose format
        {
            details.surfaceFormat = details.formats[0];

            for (auto const& availableFormat : details.formats)
            {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    details.surfaceFormat = availableFormat;
                }
            }

            details.format = details.surfaceFormat.format;
        }

        // Choose present mode
        {
            bool presentModeChosen = false;

            if (!m_vsync)
            {
                char const* logMessage = nullptr;

                for (auto const& availablePresentMode : details.presentModes)
                {
                    if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                    {
                        details.presentMode = availablePresentMode;
                        logMessage          = "Surface vsync set to off (Immediate mode)";
                        presentModeChosen   = true;  // NOLINT(clang-analyzer-deadcode.DeadStores)
                        break;
                    }
                    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                    {
                        details.presentMode = availablePresentMode;
                        logMessage          = "Surface vsync set to off (Mailbox mode)";
                        presentModeChosen   = true;  // NOLINT(clang-analyzer-deadcode.DeadStores)
                    }
                }

                logger::debug(logMessage);
            }
            else if (!presentModeChosen)
            {
                details.presentMode = VK_PRESENT_MODE_FIFO_KHR;

                if (!m_vsync)
                {
                    logger::debug("Immediate and mailbox present modes are not supported. "
                                  "Using FIFO.");
                }
                else
                {
                    logger::debug("V-Sync enabled");
                }
            }
        }

        return details;
    }
}  // namespace renderer::backend
