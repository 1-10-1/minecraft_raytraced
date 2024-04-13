#pragma once

#include <mc/utils.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class Vertex
    {
    public:
        glm::vec2 position;
        glm::vec3 color;

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
                    .format   = VK_FORMAT_R32G32_SFLOAT,
                    .offset   = utils::member_offset(&Vertex::position),
                },
                {
                    .location = 1,
                    .binding  = 0,
                    .format   = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset   = utils::member_offset(&Vertex::color),
                }
            });
            // clang-format on
        }
    };

    // clang-format off

    std::vector<Vertex> const vertices = {
        { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } },
        { { -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f } },
    };

    std::vector<uint32_t> const indices = {
        0, 1, 2, 2, 3, 0
    };
    // clang-format on

}  // namespace renderer::backend
