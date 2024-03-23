#pragma once

#include "backend/backend_renderer.hpp"

namespace renderer
{
    class Renderer
    {
    public:
        Renderer();

        void render();

    private:
        backend::BackendRenderer backend;
    };
}  // namespace renderer
