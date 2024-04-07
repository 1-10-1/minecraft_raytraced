#pragma once

#include <mc/utils.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class VertexLayout
    {
    public:
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec3 tangent;
        glm::vec3 bitangent;

        static auto getBindingDescription() -> VkVertexInputBindingDescription
        {
            return { .binding = 0, .stride = sizeof(VertexLayout), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
        }

        static auto getAttributeDescriptions()
        {
            return std::to_array<VkVertexInputAttributeDescription>({
                {
                 .location = 0,
                 .binding  = 0,
                 .format   = VK_FORMAT_R32G32B32_SFLOAT,
                 .offset   = Utils::member_offset<uint32_t>(&VertexLayout::position),
                 },
                {
                 .location = 1,
                 .binding  = 0,
                 .format   = VK_FORMAT_R32G32B32_SFLOAT,
                 .offset   = Utils::member_offset<uint32_t>(&VertexLayout::normal),
                 },
                {
                 .location = 2,
                 .binding  = 0,
                 .format   = VK_FORMAT_R32G32_SFLOAT,
                 .offset   = Utils::member_offset<uint32_t>(&VertexLayout::texCoord),
                 },
                {
                 .location = 3,
                 .binding  = 0,
                 .format   = VK_FORMAT_R32G32B32_SFLOAT,
                 .offset   = Utils::member_offset<uint32_t>(&VertexLayout::tangent),
                 },
                {
                 .location = 4,
                 .binding  = 0,
                 .format   = VK_FORMAT_R32G32B32_SFLOAT,
                 .offset   = Utils::member_offset<uint32_t>(&VertexLayout::bitangent),
                 }
            });
        }
    };
}  // namespace renderer::backend
