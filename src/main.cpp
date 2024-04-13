#include "mc/events.hpp"
#include <mc/exceptions.hpp>
#include <mc/game/game.hpp>
#include <mc/game/loop.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/renderer.hpp>

#include <array>
#include <filesystem>

#include <tracy/Tracy.hpp>

#ifdef __linux__
#    include <libgen.h>
#    include <linux/limits.h>
#    include <unistd.h>
#endif

void switchCwd();

auto main() -> int
{
    switchCwd();

    [[maybe_unused]] std::string_view appName = "Minecraft Clone Game";
    TracyAppInfo(appName.data(), appName.size());

    logger::Logger::init();

    EventManager eventManager {};
    window::Window window { eventManager };
    renderer::Renderer m_renderer { eventManager, window };
    game::Game game {};

    MC_TRY
    {
        while (!window.shouldClose())
        {
            window::Window::pollEvents();

            eventManager.dispatchEvent(AppUpdateEvent {});
            eventManager.dispatchEvent(AppRenderEvent {});

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
