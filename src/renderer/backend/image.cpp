#include <mc/asserts.hpp>
#include <mc/exceptions.hpp>
#include <mc/renderer/backend/image.hpp>
#include <mc/renderer/backend/vk_checker.hpp>

#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace vi = std::ranges::views;

namespace
{
    using namespace renderer::backend;

    void transitionImageLayout(ScopedCommandBuffer& commandBuffer,
                               VkImage image,
                               VkFormat format,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               uint32_t mipLevels)
    {
        VkPipelineStageFlags sourceStage {};
        VkPipelineStageFlags destinationStage {};

        VkImageMemoryBarrier barrier {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout           = oldLayout,
            .newLayout           = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = image,
            .subresourceRange    = {
                .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel    = 0,
                .levelCount      = mipLevels,
                .baseArrayLayer  = 0,
                .layerCount      = 1,
            },
        };

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            MC_THROW Error(RendererError, "Unsupported layout transition");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

}  // namespace

namespace renderer::backend
{
    StbiImage::StbiImage(std::string_view path)
    {
        int texWidth { 0 };
        int texHeight { 0 };
        int texChannels { 0 };

        // Tip: instead of memcpying to the mapped gpu buffer region, make this
        // directly load it into the mapped region.
        m_data = stbi_load(path.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        m_dimensions = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) };

        m_size = static_cast<size_t>(texWidth) * texHeight * 4;  // 4 channels, RGBA

        MC_ASSERT_MSG(m_data, "Failed to load texture");
    }

    StbiImage::~StbiImage()
    {
        stbi_image_free(m_data);
    }

    Image::Image(Device& device,
                 CommandManager& commandController,
                 glm::uvec2 dimensions,
                 VkFormat format,
                 VkSampleCountFlagBits sampleCount,
                 uint32_t mipLevels,
                 VkImageUsageFlagBits usageFlags,
                 VkImageAspectFlagBits aspectFlags)
        : m_device { device },
          m_commandManager { commandController },
          m_format { format },
          m_sampleCount { sampleCount },
          m_usageFlags { usageFlags },
          m_aspectFlags { aspectFlags },
          m_mipLevels { mipLevels },
          m_dimensions { dimensions }
    {
        create();
    }

    Image::~Image()
    {
        destroy();
    };

    void Image::create()
    {
        createImage(m_format,
                    VK_IMAGE_TILING_OPTIMAL,
                    m_usageFlags,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    m_mipLevels,
                    m_sampleCount);

        createImageView(m_format, m_aspectFlags, 1);
    }

    void Image::destroy()
    {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        vkDestroyImage(m_device, m_handle, nullptr);
        vkFreeMemory(m_device, m_imageMemory, nullptr);
    }

    void Image::createImage(VkFormat format,
                            VkImageTiling tiling,
                            VkImageUsageFlags usage,
                            VkMemoryPropertyFlags properties,
                            uint32_t mipLevels,
                            VkSampleCountFlagBits numSamples)
    {
        VkImageCreateInfo imageInfo {
            .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType     = VK_IMAGE_TYPE_2D,
            .format        = format,
            .extent        = {m_dimensions.x, m_dimensions.y, 1},
            .mipLevels     = mipLevels,
            .arrayLayers   = 1,
            .samples       = numSamples,
            .tiling        = tiling,
            .usage         = usage,
            .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        vkCreateImage(m_device, &imageInfo, nullptr, &m_handle) >> vkResultChecker;

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device, m_handle, &memRequirements);

        VkMemoryAllocateInfo allocInfo {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = m_device.findSuitableMemoryType(memRequirements.memoryTypeBits, properties),
        };

        vkAllocateMemory(m_device, &allocInfo, nullptr, &m_imageMemory) >> vkResultChecker;

        vkBindImageMemory(m_device, m_handle, m_imageMemory, 0);
    }

    void Image::createImageView(VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
    {
        VkImageViewCreateInfo viewInfo {
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image              = m_handle,
            .viewType           = VK_IMAGE_VIEW_TYPE_2D,
            .format             = format,
            .subresourceRange   = {
                .aspectMask     = aspectFlags,
                .baseMipLevel   = 0,
                .levelCount     = mipLevels,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            }
        };

        vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) >> vkResultChecker;
    }

    Texture::Texture(Device& device, CommandManager& commandManager, StbiImage const& stbiImage)
        : m_device { device },
          m_commandManager { commandManager },

          m_mipLevels { static_cast<uint32_t>(
                            std::floor(std::log2(std::max(stbiImage.getDimensions().x, stbiImage.getDimensions().y)))) +
                        1 },

          m_sampler { VK_NULL_HANDLE },
          m_buffer { m_device, m_commandManager, stbiImage.getData(), stbiImage.getDataSize() },

          m_image { m_device,
                    m_commandManager,
                    stbiImage.getDimensions(),
                    VK_FORMAT_R8G8B8A8_SRGB,
                    VK_SAMPLE_COUNT_1_BIT,
                    m_mipLevels,
                    static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                    VK_IMAGE_ASPECT_COLOR_BIT }
    {
        glm::uvec2 dimensions = stbiImage.getDimensions();

        {
            ScopedCommandBuffer commandBuffer(
                m_device, m_commandManager.getGraphicsCmdPool(), m_device.getGraphicsQueue());

            transitionImageLayout(commandBuffer,
                                  m_image,
                                  VK_FORMAT_R8G8B8A8_SRGB,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  m_mipLevels);

            VkBufferImageCopy region {
                .bufferOffset      = 0,
                .bufferRowLength   = 0,
                .bufferImageHeight = 0,
                .imageSubresource  = {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel       = 0,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
                .imageOffset = { 0, 0, 0 },
                .imageExtent = { dimensions.x, dimensions.y, 1 },
            };

            vkCmdCopyBufferToImage(commandBuffer, m_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL here
            generateMipmaps(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB, m_mipLevels);
        }

        createSamplers();
    }

    Texture::~Texture()
    {
        vkDestroySampler(m_device, m_sampler, nullptr);
    }

    void Texture::createSamplers()
    {
        VkPhysicalDeviceProperties const& deviceProperties = m_device.getDeviceProperties();

        static VkSamplerCreateInfo samplerInfo {
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter               = VK_FILTER_LINEAR,
            .minFilter               = VK_FILTER_LINEAR,
            .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias              = 0.0f,
            .anisotropyEnable        = VK_TRUE,
            .maxAnisotropy           = deviceProperties.limits.maxSamplerAnisotropy,
            .compareEnable           = VK_FALSE,
            .minLod                  = 0.0f,
            .maxLod                  = static_cast<float>(m_mipLevels),
            .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);
    }

    void Texture::generateMipmaps(ScopedCommandBuffer& commandBuffer, VkFormat imageFormat, uint32_t mipLevels)
    {
        if ((m_device.getFormatProperties(imageFormat).optimalTilingFeatures &
             VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0)
        {
            MC_THROW Error(RendererError, "Texture image format does not support linear blitting!");
        }

        VkImageMemoryBarrier barrier {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = m_image,
        .subresourceRange    = {
            .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount      = 1,
            .baseArrayLayer  = 0,
            .layerCount      = 1,
        }
    };

        auto mipWidth  = static_cast<int32_t>(m_image.getDimensions().x);
        auto mipHeight = static_cast<int32_t>(m_image.getDimensions().y);

        // WARNING: Check if this is equivalent to a (1 -> miplevel) for loop
        for (uint32_t i = 1; i < mipLevels; ++i)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            // clang-format off
        VkImageBlit blit{
            .srcSubresource     = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = i - 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .srcOffsets         = { { 0, 0, 0 }, { mipWidth, mipHeight, 1 } },
            .dstSubresource     = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = i,
                .baseArrayLayer = 0,
                .layerCount     = 1
            },
            .dstOffsets         = { { 0, 0, 0 }, { mipWidth  > 1 ? mipWidth  / 2 : 1,
                                                   mipHeight > 1 ? mipHeight / 2 : 1,
                                                   1 } },
        };
            // clang-format on

            vkCmdBlitImage(commandBuffer,
                           m_image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           m_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            mipWidth /= (mipWidth > 1) ? 2 : 1;
            mipHeight /= (mipHeight > 1) ? 2 : 1;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);
    }
}  // namespace renderer::backend
