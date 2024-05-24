#pragma once

#include "mc/renderer/backend/vk_checker.hpp"
#include "mc/utils.hpp"

#include <vulkan/vulkan_core.h>

#include <filesystem>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace renderer::backend
{
    inline auto createShaderModule(vk::raii::Device const& device, std::filesystem::path const& shaderPath)
        -> vk::raii::ShaderModule
    {
        std::vector<char> shaderCode = utils::readBytes(shaderPath);

        return device.createShaderModule({
                   .codeSize = shaderCode.size(),
                   .pCode    = reinterpret_cast<uint32_t const*>(shaderCode.data()),
               }) >>
               ResultChecker();
    };
}  // namespace renderer::backend
