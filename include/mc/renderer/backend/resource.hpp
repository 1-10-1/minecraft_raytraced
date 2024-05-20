#pragma once

#include <tuple>

#include "allocator.hpp"
#include "device.hpp"

#include <vulkan/vulkan.h>

namespace renderer::backend
{
    using VulkanResourceTypes = std::tuple<VkBuffer,
                                           VkImage,
                                           VkInstance,
                                           VkPhysicalDevice,
                                           VkDevice,
                                           VkQueue,
                                           VkSemaphore,
                                           VkCommandBuffer,
                                           VkFence,
                                           VkDeviceMemory,
                                           VkEvent,
                                           VkQueryPool,
                                           VkBufferView,
                                           VkImageView,
                                           VkShaderModule,
                                           VkPipelineCache,
                                           VkPipelineLayout,
                                           VkPipeline,
                                           VkRenderPass,
                                           VkDescriptorSetLayout,
                                           VkSampler,
                                           VkDescriptorSet,
                                           VkDescriptorPool,
                                           VkFramebuffer,
                                           VkCommandPool>;

    template<typename T, typename Tuple>
    struct isVkResourceType;

    template<typename T>
    struct isVkResourceType<T, std::tuple<>> : std::false_type
    {
    };

    template<typename T, typename U, typename... Us>
    struct isVkResourceType<T, std::tuple<U, Us...>>
        : std::conditional_t<std::is_same_v<T, U>, std::true_type, isVkResourceType<T, std::tuple<Us...>>>
    {
    };

    template<typename T>
    constexpr bool isVkResource = isVkResourceType<T, VulkanResourceTypes>::value;

    template<typename T>
    class VulkanResource
    {
        static_assert(isVkResource<T*>);

    public:
        VulkanResource() = default;

        VulkanResource(Device const& device, T resource)
        {
            m_device   = &device;
            m_resource = resource;
        };

        ~VulkanResource() { destroy(); }

        VulkanResource(VulkanResource const&)                    = delete;
        auto operator=(VulkanResource const&) -> VulkanResource& = delete;

        auto operator=(VulkanResource&& other) noexcept -> VulkanResource&
        {
            if (this == &other)
            {
                return *this;
            }

            destroy();

            m_resource       = other.m_resource;
            other.m_resource = nullptr;

            return *this;
        }

        VulkanResource(VulkanResource&& other) noexcept : m_device { other.m_device }, m_resource { other.m_resource }
        {
            other.m_resource = nullptr;
        };

        T get() const { return m_resource; }

        operator T() const { return m_resource; }

        void destroy()
        {
            if (!m_resource)
            {
                return;
            }

            if constexpr (std::is_same_v<T, VkImage>)
            {
                vkDestroyImage(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkInstance>)
            {
                vkDestroyInstance(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkPhysicalDevice>)
            {
                vkDestroyPhysicalDevice(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkDevice>)
            {
                vkDestroyDevice(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkQueue>)
            {
                vkDestroyQueue(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkSemaphore>)
            {
                vkDestroySemaphore(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkCommandBuffer>)
            {
                vkDestroyCommandBuffer(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkFence>)
            {
                vkDestroyFence(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkDeviceMemory>)
            {
                vkDestroyDeviceMemory(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkEvent>)
            {
                vkDestroyEvent(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkQueryPool>)
            {
                vkDestroyQueryPool(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkBufferView>)
            {
                vkDestroyBufferView(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkImageView>)
            {
                vkDestroyImageView(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkShaderModule>)
            {
                vkDestroyShaderModule(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkPipelineCache>)
            {
                vkDestroyPipelineCache(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkPipelineLayout>)
            {
                vkDestroyPipelineLayout(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkPipeline>)
            {
                vkDestroyPipeline(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkRenderPass>)
            {
                vkDestroyRenderPass(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkDescriptorSetLayout>)
            {
                vkDestroyDescriptorSetLayout(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkSampler>)
            {
                vkDestroySampler(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkDescriptorSet>)
            {
                vkDestroyDescriptorSet(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkDescriptorPool>)
            {
                vkDestroyDescriptorPool(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkFramebuffer>)
            {
                vkDestroyFramebuffer(m_device, m_resource, nullptr);
            }
            else if constexpr (std::is_same_v<T, VkCommandPool>)
            {
                vkDestroyCommandPool(m_device, m_resource, nullptr);
            };

            m_resource = nullptr;
        }

    private:
        Device const* m_device;

        T m_resource;
    };

    template<>
    class VulkanResource<VkBuffer>
    {
    public:
        VulkanResource() = default;

        VulkanResource(Allocator const& allocator, VmaAllocation allocation, VkBuffer buffer)
            : m_allocator { &allocator }, m_allocation { allocation }, m_resource { buffer } {};

        ~VulkanResource() { destroy(); }

        VulkanResource(VulkanResource const&)                    = delete;
        auto operator=(VulkanResource const&) -> VulkanResource& = delete;

        auto operator=(VulkanResource&& other) noexcept -> VulkanResource&
        {
            if (this == &other)
            {
                return *this;
            }

            destroy();

            m_resource   = other.m_resource;
            m_allocation = other.m_allocation;
            m_allocator  = other.m_allocator;

            other.m_resource   = nullptr;
            other.m_allocator  = nullptr;
            other.m_allocation = nullptr;

            return *this;
        }

        VulkanResource(VulkanResource&& other) noexcept
            : m_allocator { other.m_allocator }, m_allocation { other.m_allocation }, m_resource { other.m_resource }
        {
            other.m_resource   = nullptr;
            other.m_allocator  = nullptr;
            other.m_allocation = nullptr;
        };

        VkBuffer get() const { return m_resource; }

        operator VkBuffer() const { return m_resource; }

        void destroy()
        {
            if (!m_resource)
            {
                return;
            }

            vmaDestroyBuffer(*m_allocator, m_resource, m_allocation);

            m_resource = nullptr;
        }

    private:
        Allocator const* m_allocator { nullptr };
        VmaAllocation m_allocation { nullptr };
        VkBuffer m_resource { nullptr };
    };
}  // namespace renderer::backend
