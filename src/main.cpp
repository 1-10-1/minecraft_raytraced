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

#if PROFILED
// NOLINTBEGIN
void* operator new(size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc(ptr, count);
    return ptr;
}
#endif

void operator delete(void* ptr) noexcept
{
    TracyFree(ptr);
    free(ptr);
}

// NOLINTEND

void switchCwd();

auto main() -> int
{
    switchCwd();

    [[maybe_unused]] std::string_view appInfo = "Minecraft Clone Game";
    TracyAppInfo(appInfo.data(), appInfo.size());

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
#ifdef __linux__
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
#else
{
}
#endif
