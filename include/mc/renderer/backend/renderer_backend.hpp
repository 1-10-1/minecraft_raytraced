#pragma once

#include "device.hpp"
#include "instance.hpp"
#include "surface.hpp"
#include "swapchain.hpp"

#include <GLFW/glfw3.h>
#include <glm/ext/vector_uint2.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class RendererBackend
    {
    public:
        explicit RendererBackend(GLFWwindow* window, glm::uvec2 initialFramebufferDimensions);

        void render();

        void recreate_surface(glm::uvec2 dimensions)
        {
            m_surface.refresh(m_device, { .width = dimensions.x, .height = dimensions.y });
        }

    private:
        Instance m_instance;
        Surface m_surface;
        Device m_device;
        Swapchain m_swapchain;
    };
}  // namespace renderer::backend
