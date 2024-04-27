#pragma once

#include "allocator.hpp"
#include "command.hpp"
#include "device.hpp"
#include "mc/asserts.hpp"

#include <string>
#include <string_view>

#include <glm/ext/vector_uint2.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class StbiImage
    {
    public:
        // NOLINTNEXTLINE(google-explicit-constructor)
        StbiImage(std::string_view path);
        ~StbiImage();

        StbiImage(StbiImage const&)                    = delete;
        StbiImage(StbiImage&&)                         = delete;
        auto operator=(StbiImage const&) -> StbiImage& = delete;
        auto operator=(StbiImage&&) -> StbiImage&      = delete;

        [[nodiscard]] auto getDimensions() const -> VkExtent2D { return m_dimensions; }

        [[nodiscard]] auto getData() const -> unsigned char const* { return m_data; }

        [[nodiscard]] auto getDataSize() const -> size_t { return m_size; }

    private:
        VkExtent2D m_dimensions {};
        size_t m_size {};
        unsigned char* m_data { nullptr };
    };

    class Image
    {
    public:
        Image(Device const& device,
              Allocator const& allocator,
              VkExtent2D dimensions,
              VkFormat format,
              VkSampleCountFlagBits sampleCount,
              VkImageUsageFlagBits usageFlags,
              VkImageAspectFlagBits aspectFlags,
              uint32_t mipLevels = 1);

        ~Image();

        auto operator=(Image const&) -> Image& = delete;
        Image(Image const&)                    = delete;
        auto operator=(Image&&) -> Image&      = delete;
        Image(Image&&)                         = delete;

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VkImage() const { return m_handle; }

        [[nodiscard]] auto getImageView() const -> VkImageView
        {
            MC_ASSERT_MSG(m_imageView,
                          "Image view is not present, probably because the image is being used for transfer only.");

            return m_imageView;
        }

        [[nodiscard]] auto getDimensions() const -> VkExtent2D { return m_dimensions; }

        [[nodiscard]] auto getMipLevels() const -> uint32_t { return m_mipLevels; }

        [[nodiscard]] auto getFormat() const -> VkFormat { return m_format; }

        void copyTo(VkCommandBuffer cmdBuf, VkImage dst, VkExtent2D dstSize, VkExtent2D offset);
        void resolveTo(VkCommandBuffer cmdBuf, VkImage dst, VkExtent2D dstSize, VkExtent2D offset);

        static void
        transition(VkCommandBuffer cmdBuf, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

        void resize(Allocator const& allocator, VkExtent2D dimensions)
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

        void create();
        void destroy();

        Device const& m_device;
        Allocator const& m_allocator;

        VkImage m_handle {};
        VkImageView m_imageView {};
        VmaAllocation m_allocation {};

        VkFormat m_format;
        VkSampleCountFlagBits m_sampleCount;
        VkImageUsageFlagBits m_usageFlags;
        VkImageAspectFlagBits m_aspectFlags;

        uint32_t m_mipLevels;

        VkExtent2D m_dimensions;
    };

    class Texture
    {
    public:
        Texture(Device& device, Allocator& allocator, CommandManager& commandManager, StbiImage const& stbiImage);
        Texture(Device& device,
                Allocator& allocator,
                CommandManager& commandManager,
                VkExtent2D dimensions,
                void* data,
                size_t dataSize);

        Texture(Texture const&)                    = delete;
        Texture(Texture&&)                         = delete;
        auto operator=(Texture&&) -> Texture&      = delete;
        auto operator=(Texture const&) -> Texture& = delete;

        ~Texture();

        [[nodiscard]] auto getPath() const -> std::string const& { return m_path; }

        [[nodiscard]] auto getSampler() const -> VkSampler { return m_sampler; }

        [[nodiscard]] auto getImageView() const -> VkImageView { return m_image.getImageView(); }

        [[nodiscard]] auto getImage() const -> Image const& { return m_image; }

    private:
        Device& m_device;
        Allocator& m_allocator;
        CommandManager& m_commandManager;

        void createSamplers(uint32_t mipLevels);

        void generateMipmaps(ScopedCommandBuffer& commandBuffer,
                             VkImage image,
                             VkExtent2D dimensions,
                             VkFormat imageFormat,
                             uint32_t mipLevels);

        std::string m_path;

        Image m_image;

        VkSampler m_sampler { VK_NULL_HANDLE };
    };

}  // namespace renderer::backend
