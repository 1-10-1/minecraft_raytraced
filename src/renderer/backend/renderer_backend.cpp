#include <mc/logger.hpp>
#include <mc/renderer/backend/renderer_backend.hpp>

#include <algorithm>
#include <limits>

namespace renderer::backend
{
    RendererBackend::RendererBackend(GLFWwindow* window, glm::uvec2 initialFramebufferDimensions)
        : m_window { window },
          m_surface { window, m_instance.get() },
          m_device { Device(m_instance.get(), m_surface) }
    {
        m_surface.refresh(
            m_device.getPhysical(),
            { .width = initialFramebufferDimensions.x, .height = initialFramebufferDimensions.y });
    }

    void Surface::refresh(VkPhysicalDevice device, VkExtent2D dimensions)
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &m_details.capabilities);

        uint32_t formatCount {};

        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            m_details.formats.resize(formatCount);

            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device, m_surface, &formatCount, m_details.formats.data());
        }

        uint32_t presentModeCount {};
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            m_details.presentModes.resize(presentModeCount);

            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, m_surface, &presentModeCount, m_details.presentModes.data());
        }

        // Choose extent
        {
            if (m_details.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                m_details.extent = { m_details.capabilities.currentExtent.width,
                                     m_details.capabilities.currentExtent.height };
            }

            m_details.extent = { std::clamp(dimensions.width,
                                            m_details.capabilities.minImageExtent.width,
                                            m_details.capabilities.maxImageExtent.width),

                                 std::clamp(dimensions.height,
                                            m_details.capabilities.minImageExtent.height,
                                            m_details.capabilities.maxImageExtent.height) };
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
                char const* logMessage = nullptr;

                for (auto const& availablePresentMode : m_details.presentModes)
                {
                    if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                    {
                        m_details.presentMode = availablePresentMode;
                        logMessage            = "Surface vsync set to off (Immediate mode)";
                        presentModeChosen     = true;  // NOLINT(clang-analyzer-deadcode.DeadStores)
                        break;
                    }
                    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                    {
                        m_details.presentMode = availablePresentMode;
                        logMessage            = "Surface vsync set to off (Mailbox mode)";
                        presentModeChosen     = true;  // NOLINT(clang-analyzer-deadcode.DeadStores)
                    }
                }

                logger::debug(logMessage);
            }
            else if (!presentModeChosen)
            {
                m_details.presentMode = VK_PRESENT_MODE_FIFO_KHR;

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
    }
}  // namespace renderer::backend
