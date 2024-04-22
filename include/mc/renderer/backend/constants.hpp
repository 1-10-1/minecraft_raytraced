#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    constexpr uint32_t kNumFramesInFlight  = 3;
    constexpr VkFormat kDepthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
}  // namespace renderer::backend
