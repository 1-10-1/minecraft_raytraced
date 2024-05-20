#include "fastgltf/types.hpp"
#include "mc/renderer/backend/image.hpp"
#include <mc/events.hpp>
#include <mc/exceptions.hpp>
#include <mc/game/game.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/renderer.hpp>
#include <mc/timer.hpp>

#include <array>
#include <filesystem>

#include <tracy/Tracy.hpp>

#ifdef __linux__
#    include <libgen.h>
#    include <linux/limits.h>
#    include <unistd.h>
#endif

// ******************************************************************************************************************************
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <filesystem>

namespace fs = std::filesystem;
namespace fg = fastgltf;

namespace renderer::backend
{
    struct GltfSceneResources
    {
        std::vector<Texture> textures;
    };

    void processGltf()
    {
        fs::path gltfFile = "../../khrSampleModels/2.0/Cube/glTF/Cube.gltf";

        if (!fs::exists(gltfFile))
        {
            MC_THROW Error(AssetError, std::format("glTF file {} not found", gltfFile.c_str()));
        }

        fg::Parser parser;

        fg::GltfDataBuffer buffer;
        buffer.loadFromFile(gltfFile);

        GltfSceneResources resources;

        fg::Asset asset;

        {
            Timer timer;

            fg::Expected<fg::Asset> expected = parser.loadGltfJson(&buffer, gltfFile.parent_path());

            if (!expected)
            {
                MC_THROW Error(AssetError,
                               std::format("Could not parse gltf file {}: {}",
                                           gltfFile.root_name().c_str(),
                                           magic_enum::enum_name(expected.error())));
            }

            logger::debug("Parsed {} successfully! ({})", gltfFile.root_name().c_str(), timer.getTotalTime());

            asset = std::move(expected.get());
        }

        // *********************************************************************************************************************
        // TEXTURES

        // resources.textures.reserve(asset.textures.size());
    }

}  // namespace renderer::backend

// ******************************************************************************************************************************

void switchCwd();

auto main() -> int
{
    switchCwd();

    [[maybe_unused]] std::string_view appName = "Minecraft Clone Game";
    TracyAppInfo(appName.data(), appName.size());

    logger::Logger::init();

    EventManager eventManager {};
    window::Window window { eventManager };
    Camera camera;
    renderer::Renderer m_renderer { eventManager, window, camera };
    game::Game game { eventManager, window, camera };

    eventManager.subscribe(&camera, &Camera::onUpdate, &Camera::onFramebufferResize);

    renderer::backend::processGltf();

    MC_TRY
    {
        Timer timer;

        while (!window.shouldClose())
        {
            window::Window::pollEvents();

            eventManager.dispatchEvent(AppUpdateEvent { timer });
            eventManager.dispatchEvent(AppRenderEvent {});

            timer.tick();

            FrameMark;
        }
    }
    MC_CATCH(...)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void switchCwd()
{
#ifdef __linux__
    std::array<char, PATH_MAX> result {};
    ssize_t count = readlink("/proc/self/exe", result.data(), PATH_MAX);

    std::string_view path;

    if (count != -1)
    {
        path = dirname(result.data());
    }

    std::filesystem::current_path(path);
#endif
}
