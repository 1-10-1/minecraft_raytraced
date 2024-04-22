#include <mc/events.hpp>
#include <mc/exceptions.hpp>
#include <mc/game/game.hpp>
#include <mc/logger.hpp>
#include <mc/renderer/renderer.hpp>
#include <mc/timer.hpp>

#include <array>
#include <filesystem>

#include <tracy/Tracy.hpp>
#include <vulkan/vulkan.hpp>

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
    Camera camera;
    renderer::Renderer m_renderer { eventManager, window, camera };
    game::Game game { eventManager, window, camera };

    eventManager.subscribe(&camera, &Camera::onUpdate, &Camera::onFramebufferResize);

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
