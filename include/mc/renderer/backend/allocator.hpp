#pragma once

#include "device.hpp"
#include "instance.hpp"
#include "vma.hpp"

namespace renderer::backend
{
    class Allocator
    {
    public:
        Allocator(Instance const& instance, Device const& device);
        ~Allocator();

        Allocator(Allocator const&)                    = delete;
        Allocator(Allocator&&)                         = delete;
        auto operator=(Allocator const&) -> Allocator& = delete;
        auto operator=(Allocator&&) -> Allocator&      = delete;

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VmaAllocator() const { return m_allocator; }

    private:
        VmaAllocator m_allocator {};
    };
}  // namespace renderer::backend
