#pragma once

#include "backend/renderer_backend.hpp"

namespace renderer
{
    class Renderer
    {
    public:
        explicit Renderer(GLFWwindow* window);

        void render();

    private:
        backend::RendererBackend m_backend;
    };
}  // namespace renderer
