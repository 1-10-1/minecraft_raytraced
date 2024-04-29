#include <mc/asserts.hpp>
#include <mc/exceptions.hpp>
#include <mc/renderer/backend/allocator.hpp>
#include <mc/renderer/backend/buffer.hpp>
#include <mc/renderer/backend/command.hpp>
#include <mc/renderer/backend/image.hpp>
#include <mc/renderer/backend/vk_checker.hpp>

#include <ranges>

#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace vi = std::ranges::views;

namespace
{
    using namespace renderer::backend;

    // Does not follow VkGuide, used only in the old tutorial's texture creation code
    // Dont use this
    [[deprecated("Used in the old tutorial")]] void transitionImageLayout(ScopedCommandBuffer& commandBuffer,
                                                                          VkImage image,
                                                                          VkFormat format,
                                                                          VkImageLayout oldLayout,
                                                                          VkImageLayout newLayout,
                                                                          uint32_t mipLevels);
}  // namespace

namespace renderer::backend
{
    StbiImage::StbiImage(std::string_view const& path)
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

    Image::Image(Device const& device,
                 Allocator const& allocator,
                 VkExtent2D dimensions,
                 VkFormat format,
                 VkSampleCountFlagBits sampleCount,
                 VkImageUsageFlagBits usageFlags,
                 VkImageAspectFlagBits aspectFlags,
                 uint32_t mipLevels)
        : m_device { device },
          m_allocator { allocator },
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

        // If the image is solely being used for transfer, dont make a view
        if ((m_usageFlags & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)) < m_usageFlags)
        {
            createImageView(m_format, m_aspectFlags, 1);
        }
    }

    void Image::destroy()
    {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        vmaDestroyImage(m_allocator, m_handle, m_allocation);
    }

    void Image::resolveTo(VkCommandBuffer cmdBuf, VkImage dst, VkExtent2D dstSize, VkExtent2D offset)
    {
        VkImageResolve2 resolve { .sType = VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2 };

        resolve.srcOffset.x = static_cast<int32_t>(offset.width);
        resolve.srcOffset.y = static_cast<int32_t>(offset.height);
        resolve.srcOffset.z = 0;

        resolve.dstOffset.x = 0;
        resolve.dstOffset.y = 0;
        resolve.dstOffset.z = 0;

        resolve.extent.width  = dstSize.width;
        resolve.extent.height = dstSize.height;
        resolve.extent.depth  = 1;

        resolve.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        resolve.srcSubresource.baseArrayLayer = 0;
        resolve.srcSubresource.layerCount     = 1;
        resolve.srcSubresource.mipLevel       = 0;

        resolve.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        resolve.dstSubresource.baseArrayLayer = 0;
        resolve.dstSubresource.layerCount     = 1;
        resolve.dstSubresource.mipLevel       = 0;

        VkResolveImageInfo2 resolveInfo { .sType = VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2 };
        resolveInfo.dstImage       = dst;
        resolveInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        resolveInfo.srcImage       = m_handle;
        resolveInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        resolveInfo.regionCount    = 1;
        resolveInfo.pRegions       = &resolve;

        vkCmdResolveImage2(cmdBuf, &resolveInfo);
    }

    void Image::copyTo(VkCommandBuffer cmdBuf, VkImage dst, VkExtent2D dstSize, VkExtent2D offset)
    {
        VkImageBlit2 blitRegion { .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

        blitRegion.srcOffsets[1].x = static_cast<int32_t>(offset.width);
        blitRegion.srcOffsets[1].y = static_cast<int32_t>(offset.height);
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = static_cast<int32_t>(dstSize.width);
        blitRegion.dstOffsets[1].y = static_cast<int32_t>(dstSize.height);
        blitRegion.dstOffsets[1].z = 1;

        blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount     = 1;
        blitRegion.srcSubresource.mipLevel       = 0;

        blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount     = 1;
        blitRegion.dstSubresource.mipLevel       = 0;

        VkBlitImageInfo2 blitInfo { .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
        blitInfo.dstImage       = dst;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.srcImage       = m_handle;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.filter         = VK_FILTER_LINEAR;
        blitInfo.regionCount    = 1;
        blitInfo.pRegions       = &blitRegion;

        vkCmdBlitImage2(cmdBuf, &blitInfo);
    };

    void Image::transition(VkCommandBuffer cmdBuf, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
    {
        // TODO(aether) Those stage masks will cause the pipeline to stall
        // figure out the appropriate stages based on a new parameter or the ones already given
        VkImageMemoryBarrier2 imageBarrier {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .srcAccessMask    = VK_ACCESS_2_MEMORY_WRITE_BIT,
            .dstStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask    = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
            .oldLayout        = currentLayout,
            .newLayout        = newLayout,
            .image            = image,
            .subresourceRange = {
                .aspectMask     = static_cast<uint32_t>((newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                  ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
                .baseMipLevel   = 0,
                .levelCount     = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount     = VK_REMAINING_ARRAY_LAYERS,
            },

        };

        VkDependencyInfo depInfo {
            .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &imageBarrier,
        };

        vkCmdPipelineBarrier2(cmdBuf, &depInfo);
    };

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
            .extent        = { m_dimensions.width, m_dimensions.height, 1 },
            .mipLevels     = mipLevels,
            .arrayLayers   = 1,
            .samples       = numSamples,
            .tiling        = tiling,
            .usage         = usage,
            .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VmaAllocationCreateInfo imageAllocInfo = {
            .usage         = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };

        vmaCreateImage(m_allocator, &imageInfo, &imageAllocInfo, &m_handle, &m_allocation, nullptr);
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

    // Textures are done I think!
    Texture::Texture(Device& device, Allocator& allocator, CommandManager& commandManager, StbiImage const& stbiImage)
        : m_device { device },
          m_allocator { allocator },
          m_commandManager { commandManager },
          m_image { m_device,
                    allocator,
                    stbiImage.getDimensions(),
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_SAMPLE_COUNT_1_BIT,
                    static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    static_cast<uint32_t>(std::floor(
                        std::log2(std::max(stbiImage.getDimensions().width, stbiImage.getDimensions().height)))) +
                        1 }
    {
        VkExtent2D dimensions = stbiImage.getDimensions();

        uint32_t mipLevels = m_image.getMipLevels();

        BasicBuffer uploadBuffer(
            m_allocator, stbiImage.getDataSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        std::memcpy(uploadBuffer.getMappedData(), stbiImage.getData(), stbiImage.getDataSize());

        {
            ScopedCommandBuffer commandBuffer(
                m_device, m_commandManager.getGraphicsCmdPool(), m_device.getGraphicsQueue());

            Image::transition(commandBuffer, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            transitionImageLayout(commandBuffer,
                                  m_image,
                                  VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  mipLevels);

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
                .imageExtent = { dimensions.width, dimensions.height, 1 },
            };

            vkCmdCopyBufferToImage(
                commandBuffer, uploadBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL here
            generateMipmaps(
                commandBuffer, m_image, { dimensions.width, dimensions.height }, VK_FORMAT_R8G8B8A8_SRGB, mipLevels);
        }

        createSamplers(mipLevels);
    }

    Texture::Texture(Device& device,
                     Allocator& allocator,
                     CommandManager& commandManager,
                     VkExtent2D dimensions,
                     void* data,
                     size_t dataSize)
        : m_device { device },
          m_allocator { allocator },
          m_commandManager { commandManager },
          m_image { m_device,
                    allocator,
                    dimensions,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_SAMPLE_COUNT_1_BIT,
                    static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    static_cast<uint32_t>(std::floor(std::log2(std::max(dimensions.width, dimensions.height)))) + 1 }
    {
        uint32_t mipLevels = m_image.getMipLevels();

        BasicBuffer uploadBuffer(m_allocator, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        std::memcpy(uploadBuffer.getMappedData(), data, dataSize);

        {
            ScopedCommandBuffer commandBuffer(
                m_device, m_commandManager.getGraphicsCmdPool(), m_device.getGraphicsQueue());

            Image::transition(commandBuffer, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            transitionImageLayout(commandBuffer,
                                  m_image,
                                  VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  mipLevels);

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
                .imageExtent = { dimensions.width, dimensions.height, 1 },
            };

            vkCmdCopyBufferToImage(
                commandBuffer, uploadBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL here
            generateMipmaps(
                commandBuffer, m_image, { dimensions.width, dimensions.height }, VK_FORMAT_R8G8B8A8_SRGB, mipLevels);
        }

        createSamplers(mipLevels);
    }

    Texture::~Texture()
    {
        vkDestroySampler(m_device, m_sampler, nullptr);
    }

    void Texture::createSamplers(uint32_t mipLevels)
    {
        VkPhysicalDeviceProperties const& deviceProperties = m_device.getDeviceProperties();

        static VkSamplerCreateInfo samplerInfo {
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter               = VK_FILTER_NEAREST,
            .minFilter               = VK_FILTER_NEAREST,
            .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias              = 0.0f,
            .anisotropyEnable        = VK_TRUE,
            .maxAnisotropy           = deviceProperties.limits.maxSamplerAnisotropy,
            .compareEnable           = VK_FALSE,
            .minLod                  = 0.0f,
            .maxLod                  = static_cast<float>(mipLevels),
            .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);
    }

    void Texture::generateMipmaps(ScopedCommandBuffer& commandBuffer,
                                  VkImage image,
                                  VkExtent2D dimensions,
                                  VkFormat imageFormat,
                                  uint32_t mipLevels)
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
        .image               = image,
        .subresourceRange    = {
            .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount      = 1,
            .baseArrayLayer  = 0,
            .layerCount      = 1,
        }
    };

        int mipWidth  = static_cast<int>(dimensions.width);
        int mipHeight = static_cast<int>(dimensions.height);

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
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image,
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

namespace
{
    using namespace renderer::backend;

    [[maybe_unused]] void transitionImageLayout(ScopedCommandBuffer& commandBuffer,
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
