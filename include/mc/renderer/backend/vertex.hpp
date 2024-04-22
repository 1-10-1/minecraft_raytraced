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

    class Vertex
    {
    public:
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
        glm::vec3 tangent;
        glm::vec3 bitangent;

        static auto getBindingDescription() -> VkVertexInputBindingDescription
        {
            return { .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
        }

        static auto getAttributeDescriptions()
        {
            // clang-format off
            return std::to_array<VkVertexInputAttributeDescription>({
                {
                    .location = 0,
                    .binding  = 0,
                    .format   = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset   = utils::member_offset(&Vertex::position),
                },
                {
                    .location = 1,
                    .binding  = 0,
                    .format   = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset   = utils::member_offset(&Vertex::normal),
                },
                {
                    .location = 2,
                    .binding  = 0,
                    .format   = VK_FORMAT_R32G32_SFLOAT,
                    .offset   = utils::member_offset(&Vertex::texCoords),
                },
                {
                    .location = 3,
                    .binding  = 0,
                    .format   = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset   = utils::member_offset(&Vertex::tangent),
                },
                {
                    .location = 4,
                    .binding  = 0,
                    .format   = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset   = utils::member_offset(&Vertex::bitangent),
                }
            });
            // clang-format on
        }
    };
}  // namespace renderer::backend
