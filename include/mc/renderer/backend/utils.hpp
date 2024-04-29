#pragma once

#include "mc/utils.hpp"
#include "vk_checker.hpp"

#include <vulkan/vulkan_core.h>

#include <filesystem>
#include <vector>

namespace renderer::backend
{
    auto createShaderModule(VkDevice device, std::filesystem::path const& shaderPath) -> VkShaderModule
    {
        std::vector<char> shaderCode = utils::readBytes(shaderPath);

        VkShaderModuleCreateInfo createInfo {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shaderCode.size(),
            .pCode    = reinterpret_cast<uint32_t const*>(shaderCode.data()),
        };

        VkShaderModule shaderModule {};

        vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) >> vkResultChecker;

        return shaderModule;
    };
}  // namespace renderer::backend
