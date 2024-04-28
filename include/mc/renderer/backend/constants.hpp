#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    constexpr uint32_t kNumFramesInFlight       = 2;
    constexpr VkFormat kDepthStencilFormat      = VK_FORMAT_D32_SFLOAT;
    constexpr VkSampleCountFlagBits kMaxSamples = VK_SAMPLE_COUNT_4_BIT;
}  // namespace renderer::backend
