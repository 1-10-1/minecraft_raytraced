#include <mc/exceptions.hpp>
#include <mc/game/game.hpp>
#include <mc/game/loop.hpp>
#include <mc/logger.hpp>

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

    game::Game game {};  // TODO(aether): Game shouldn't be the top-most class

    MC_TRY
    {
        game.runLoop();
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
