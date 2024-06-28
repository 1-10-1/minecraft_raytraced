#pragma once

#include <mc/utils.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    struct alignas(16) ComputePushConstants
    {
        glm::vec4 data1 {}, data2 {}, data3 {}, data4 {};
    };
}  // namespace renderer::backend
