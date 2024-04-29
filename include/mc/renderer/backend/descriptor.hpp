#pragma once

#include <deque>
#include <span>
#include <vector>

#include <vulkan/vulkan.h>

namespace renderer::backend
{
    class DescriptorLayoutBuilder
    {
    public:
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        auto addBinding(uint32_t binding, VkDescriptorType type) -> DescriptorLayoutBuilder&
        {
            bindings.push_back({
                .binding         = binding,
                .descriptorType  = type,
                .descriptorCount = 1,
            });

            return *this;
        };

        void clear() { bindings.clear(); };

        auto build(VkDevice device, VkShaderStageFlags shaderStages) -> VkDescriptorSetLayout;
    };

    struct DescriptorWriter
    {
        std::deque<VkDescriptorImageInfo> imageInfos;
        std::deque<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkWriteDescriptorSet> writes;

        void
        write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
        void write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

        void clear();
        void update_set(VkDevice device, VkDescriptorSet set);
    };

    struct DescriptorAllocatorGrowable
    {
    public:
        struct PoolSizeRatio
        {
            VkDescriptorType type;
            float ratio;
        };

        void init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
        void clear_pools(VkDevice device);
        void destroy_pools(VkDevice device);

        auto allocate(VkDevice device, VkDescriptorSetLayout layout) -> VkDescriptorSet;

    private:
        auto get_pool(VkDevice device) -> VkDescriptorPool;
        static auto create_pool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
            -> VkDescriptorPool;

        std::vector<PoolSizeRatio> ratios;
        std::vector<VkDescriptorPool> fullPools;
        std::vector<VkDescriptorPool> readyPools;
        uint32_t setsPerPool;
    };

    class DescriptorAllocator
    {
    public:
        struct PoolSizeRatio
        {
            VkDescriptorType type;
            float ratio;
        };

        void initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);

        auto allocate(VkDevice device, VkDescriptorSetLayout layout) -> VkDescriptorSet;

        void clearDescriptors(VkDevice device) { vkResetDescriptorPool(device, m_pool, 0); }

        void destroyPool(VkDevice device) { vkDestroyDescriptorPool(device, m_pool, nullptr); };

        [[nodiscard]] auto getPool() const -> VkDescriptorPool { return m_pool; }

    private:
        VkDescriptorPool m_pool;
    };

    // class DescriptorManager
    // {
    // public:
    //     explicit DescriptorManager(Device& device,
    //                                std::array<UniformBuffer, kNumFramesInFlight> const& uniformBufferArray,
    //                                VkImageView textureImageView,
    //                                VkSampler textureSampler);
    //     ~DescriptorManager();
    //
    //     DescriptorManager(DescriptorManager const&)                    = delete;
    //     DescriptorManager(DescriptorManager&&)                         = delete;
    //     auto operator=(DescriptorManager const&) -> DescriptorManager& = delete;
    //     auto operator=(DescriptorManager&&) -> DescriptorManager&      = delete;
    //
    //     void bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, size_t currentFrame);
    //
    //     [[nodiscard]] auto getLayout() const -> VkDescriptorSetLayout { return m_layout; }
    //
    //     [[nodiscard]] auto getPool() const -> VkDescriptorPool { return m_pool; }
    //
    // private:
    //     void initLayout();
    //     void initPool();
    //     void initSets();
    //
    //     Device& m_device;
    //
    //     VkDescriptorSetLayout m_layout {};
    //     VkDescriptorPool m_pool {};
    //
    //     std::array<VkDescriptorSet, kNumFramesInFlight> m_descriptorSets {};
    // };
}  // namespace renderer::backend
