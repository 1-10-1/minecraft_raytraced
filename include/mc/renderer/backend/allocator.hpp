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
        auto operator=(Allocator const&) -> Allocator& = delete;

        Allocator(Allocator&& other) : m_allocator { other.m_allocator } { other.m_allocator = nullptr; };

        auto operator=(Allocator&& other) -> Allocator&
        {
            m_allocator       = other.m_allocator;
            other.m_allocator = nullptr;

            return *this;
        };

        // NOLINTNEXTLINE(google-explicit-constructor)
        [[nodiscard]] operator VmaAllocator() const { return m_allocator; }

        [[nodiscard]] auto get() const -> VmaAllocator { return m_allocator; }

    private:
        VmaAllocator m_allocator {};
    };
}  // namespace renderer::backend
