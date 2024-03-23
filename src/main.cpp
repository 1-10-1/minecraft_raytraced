#include <mc/exceptions.hpp>
#include <mc/game/game.hpp>
#include <mc/game/loop.hpp>
#include <mc/logger.hpp>

#include <array>
#include <filesystem>

#include <libgen.h>
#include <linux/limits.h>
#include <unistd.h>

void switchCwd();

auto main() -> int
{
    switchCwd();

    logger::Logger::init();

    game::Game game {};

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
    std::array<char, PATH_MAX> result {};
    ssize_t count = readlink("/proc/self/exe", result.data(), PATH_MAX);

    std::string_view path;

    if (count != -1)
    {
        path = dirname(result.data());
    }

    std::filesystem::current_path(path);
}
