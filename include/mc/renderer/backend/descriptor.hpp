#pragma once

#include <deque>
#include <span>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

namespace renderer::backend
{
    class DescriptorLayoutBuilder
    {
    public:
        std::vector<vk::DescriptorSetLayoutBinding> bindings;

        auto addBinding(uint32_t binding, vk::DescriptorType type) -> DescriptorLayoutBuilder&
        {
            bindings.push_back({
                .binding         = binding,
                .descriptorType  = type,
                .descriptorCount = 1,
            });

            return *this;
        };

        void clear() { bindings.clear(); };

        auto build(vk::raii::Device const& device,
                   vk::ShaderStageFlags shaderStages) -> vk::raii::DescriptorSetLayout;
    };

    struct DescriptorWriter
    {
        std::deque<vk::DescriptorImageInfo> imageInfos;
        std::deque<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::WriteDescriptorSet> writes;

        void write_image(int binding,
                         vk::ImageView image,
                         vk::Sampler sampler,
                         vk::ImageLayout layout,
                         vk::DescriptorType type);

        void
        write_buffer(int binding, vk::Buffer buffer, size_t size, size_t offset, vk::DescriptorType type);

        void clear();
        void update_set(vk::raii::Device const& device, vk::DescriptorSet set);
    };

    struct DescriptorAllocatorGrowable
    {
    public:
        struct PoolSizeRatio
        {
            vk::DescriptorType type;
            float ratio;
        };

        void init(vk::raii::Device const& device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
        void clear_pools(vk::raii::Device const& device);
        void destroy_pools(vk::raii::Device const& device);

        auto allocate(vk::raii::Device const& device,
                      vk::raii::DescriptorSetLayout const& layout) -> vk::DescriptorSet;

    private:
        auto get_pool(vk::raii::Device const& device) -> vk::raii::DescriptorPool;
        static auto create_pool(vk::raii::Device const& device,
                                uint32_t setCount,
                                std::span<PoolSizeRatio> poolRatios) -> vk::raii::DescriptorPool;

        std::vector<PoolSizeRatio> ratios;
        std::vector<vk::raii::DescriptorPool> fullPools;
        std::vector<vk::raii::DescriptorPool> readyPools;
        uint32_t setsPerPool;
    };

    class DescriptorAllocator
    {
    public:
        struct PoolSizeRatio
        {
            vk::DescriptorType type;
            float ratio;
        };

        DescriptorAllocator()  = default;
        ~DescriptorAllocator() = default;

        DescriptorAllocator(vk::raii::Device const& device,
                            uint32_t maxSets,
                            std::span<PoolSizeRatio> poolRatios);

        DescriptorAllocator(DescriptorAllocator&&)                    = default;
        auto operator=(DescriptorAllocator&&) -> DescriptorAllocator& = default;

        DescriptorAllocator(DescriptorAllocator const&)                    = delete;
        auto operator=(DescriptorAllocator const&) -> DescriptorAllocator& = delete;

        auto allocate(vk::Device device, vk::DescriptorSetLayout layout) -> vk::DescriptorSet;

        void clearDescriptors(vk::raii::Device const& device) { m_pool.reset(); }

        [[nodiscard]] auto getPool() const -> vk::raii::DescriptorPool const& { return m_pool; }

    private:
        vk::raii::DescriptorPool m_pool { nullptr };
    };
}  // namespace renderer::backend
