#pragma once

#include "allocator.hpp"
#include "device.hpp"

#include <string_view>

#include <glm/ext/vector_uint2.hpp>
#include <vk_mem_alloc.h>
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

        [[nodiscard]] auto getImageView() const -> VkImageView { return m_imageView; }

        [[nodiscard]] auto getDimensions() const -> VkExtent2D { return m_dimensions; }

        [[nodiscard]] auto getMipLevels() const -> uint32_t { return m_mipLevels; }

        [[nodiscard]] auto getFormat() const -> VkFormat { return m_format; }

        void copyTo(VkCommandBuffer cmdBuf, VkImage dst, VkExtent2D dstSize, VkExtent2D offset);

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

    // class Texture
    // {
    // public:
    //     Texture(Device& device, CommandManager& commandManager, StbiImage const& stbiImage);
    //
    //     Texture(Texture const&)                    = delete;
    //     Texture(Texture&&)                         = delete;
    //     auto operator=(Texture&&) -> Texture&      = delete;
    //     auto operator=(Texture const&) -> Texture& = delete;
    //
    //     ~Texture();
    //
    //     [[nodiscard]] auto getPath() const -> std::string const& { return m_path; }
    //
    //     [[nodiscard]] auto getSampler() const -> VkSampler { return m_sampler; }
    //
    //     [[nodiscard]] auto getImageView() const -> VkImageView { return m_image.getImageView(); }
    //
    // private:
    //     Device& m_device;
    //     CommandManager& m_commandManager;
    //
    //     void createSamplers(uint32_t mipLevels);
    //
    //     void generateMipmaps(ScopedCommandBuffer& commandBuffer,
    //                          VkImage image,
    //                          VkExtent2D dimensions,
    //                          VkFormat imageFormat,
    //                          uint32_t mipLevels);
    //
    //     std::string m_path;
    //
    //     Image m_image;
    //
    //     VkSampler m_sampler { VK_NULL_HANDLE };
    // };
    //
}  // namespace renderer::backend
