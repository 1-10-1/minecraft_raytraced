#pragma once

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class Device
    {
    public:
        Device();

    private:
        VkPhysicalDevice m_physicalDevice;
    };
}  // namespace renderer::backend
