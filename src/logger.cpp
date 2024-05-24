#include <mc/defines.hpp>
#include <mc/logger.hpp>

#include <filesystem>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

namespace logger
{
    void Logger::init()
    {
        auto logFilePath { std::filesystem::current_path() / "kevlar.log" };

        spdlog::level::level_enum level { SPDLOG_ACTIVE_LEVEL };

        std::vector<spdlog::sink_ptr> sinks { std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
                                              std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                                                  logFilePath.string(), true) };

        for (auto& sink : sinks)
        {
            sink->set_pattern(kDebug ? "%^[%l] [%r] [%g:%# %!]%$\n-> %v\n" : "%^[%l] [%r]%$\n-> %v\n");
        }

        m_logger = std::make_shared<spdlog::logger>("MAIN", sinks.begin(), sinks.end());

        spdlog::register_logger(m_logger);

        m_logger->set_level(level);

        spdlog::flush_every(2s);
    }
}  // namespace logger
