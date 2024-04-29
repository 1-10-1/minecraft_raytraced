#include <mc/exceptions.hpp>
#include <mc/utils.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <vector>

namespace utils
{
    auto readBytes(std::filesystem::path const& filepath) -> std::vector<char>
    {
        std::ifstream file(filepath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            MC_THROW Error(AssetError, std::format("Failed to read shader file at path {}", filepath.string()));
        }

        auto fileSize = file.tellg();
        std::vector<char> buffer(static_cast<std::size_t>(fileSize));

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
}  // namespace utils
