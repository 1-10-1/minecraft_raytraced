#pragma once

#include <mc/exceptions.hpp>
#include <mc/renderer/backend/buffer.hpp>
#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/device.hpp>

#include <string>

#include <glm/ext/vector_uint2.hpp>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class StbiImage
    {
    public:
        explicit StbiImage(std::string_view path);
        ~StbiImage();

        StbiImage(StbiImage const&)                    = delete;
        StbiImage(StbiImage&&)                         = delete;
        auto operator=(StbiImage const&) -> StbiImage& = delete;
        auto operator=(StbiImage&&) -> StbiImage&      = delete;

        [[nodiscard]] auto getDimensions() const -> glm::uvec2 { return m_dimensions; }

        [[nodiscard]] auto getData() const -> unsigned char const* { return m_data; }

        [[nodiscard]] auto getDataSize() const -> size_t { return m_size; }

    private:
        glm::uvec2 m_dimensions {};
        size_t m_size {};
        unsigned char* m_data { nullptr };
    };

    class Image
    {
    public:
        Image(Device& device,
              CommandManager& commandController,
              glm::uvec2 dimensions,
              VkFormat format,
              VkSampleCountFlagBits sampleCount,
              uint32_t mipLevels,
              VkImageUsageFlagBits usageFlags,
              VkImageAspectFlagBits aspectFlags);

        ~Image();

        auto operator=(Image const&) -> Image& = delete;
        Image(Image const&)                    = delete;
        auto operator=(Image&&) -> Image&      = delete;
        Image(Image&&)                         = delete;

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkImage() const { return m_handle; }

        [[nodiscard]] auto getImageView() const -> VkImageView { return m_imageView; }

        [[nodiscard]] auto getDimensions() const -> glm::uvec2 { return m_dimensions; }

        void resize(glm::uvec2 dimensions)
        {
            m_dimensions = dimensions;

            destroy();
            create();
        }

    private:
        void createImage(VkFormat format,
                         VkImageTiling tiling,
                         VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties,
                         uint32_t mipLevels,
                         VkSampleCountFlagBits numSamples);

        void createImageView(VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

        void transitionImageLayout(ScopedCommandBuffer& commandBuffer,
                                   VkFormat format,
                                   VkImageLayout oldLayout,
                                   VkImageLayout newLayout,
                                   uint32_t mipLevels);

        void create();
        void destroy();

        Device& m_device;
        CommandManager& m_commandManager;

        VkImage m_handle {};
        VkImageView m_imageView {};
        VkDeviceMemory m_imageMemory {};
        VkFormat m_format;
        VkSampleCountFlagBits m_sampleCount;
        VkImageUsageFlagBits m_usageFlags;
        VkImageAspectFlagBits m_aspectFlags;

        uint32_t m_mipLevels;

        glm::uvec2 m_dimensions {};
    };

    class Texture
    {
    public:
        Texture(Device& device, CommandManager& commandManager, StbiImage const& stbiImage);

        Texture(Texture const&)                    = delete;
        Texture(Texture&&)                         = delete;
        auto operator=(Texture&&) -> Texture&      = delete;
        auto operator=(Texture const&) -> Texture& = delete;

        ~Texture();

        [[nodiscard]] auto getSampler() const -> VkSampler { return m_sampler; }

        [[nodiscard]] auto getImageView() const -> VkImageView { return m_image.getImageView(); }

    private:
        Device& m_device;
        CommandManager& m_commandManager;

        void createSamplers();

        void generateMipmaps(ScopedCommandBuffer& commandBuffer, VkFormat imageFormat, uint32_t mipLevels);

        std::string m_path;
        uint32_t m_mipLevels {};
        VkSampler m_sampler;

        TextureBuffer m_buffer;
        Image m_image;
    };

}  // namespace renderer::backend
