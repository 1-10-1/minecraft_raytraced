#pragma once

#include "instance.hpp"

#include <vulkan/vulkan.h>

namespace renderer::backend
{
    class BackendRenderer
    {
    public:
        BackendRenderer();

        void render();

    private:
        Instance m_instance;
    };
}  // namespace renderer::backend
