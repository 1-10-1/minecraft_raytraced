#include <mc/renderer/backend/allocator.hpp>

namespace renderer::backend
{
    Allocator::Allocator(Instance const& instance, Device const& device)
    {
        VmaAllocatorCreateInfo allocatorInfo = {
            .flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .physicalDevice = device,
            .device         = device,
            .instance       = instance,
        };

        vmaCreateAllocator(&allocatorInfo, &m_allocator);
    }

    Allocator::~Allocator()
    {
        vmaDestroyAllocator(m_allocator);
    }
}  // namespace renderer::backend
