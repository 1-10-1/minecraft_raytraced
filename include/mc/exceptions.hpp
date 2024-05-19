#pragma once

#include <source_location>
#include <stdexcept>
#include <string>

#include <magic_enum.hpp>

#include "logger.hpp"

enum ErrorType
{
    GenericError,
    GraphicsError,
    ApplicationError,
    WindowError,
    RendererError,
    EventError,
    AssetError
};

class Error : public std::runtime_error
{
public:
    Error(ErrorType type, std::string const& msg, std::source_location loc = std::source_location::current())
        : std::runtime_error { std::format("[{}] {}", magic_enum::enum_name(type), msg) }, m_location(loc)
    {
    }

    ~Error() override = default;

    Error(Error const&) = default;
    Error(Error&&)      = default;

    auto operator=(Error const&) -> Error const& = delete;
    auto operator=(Error&&) -> Error const&      = delete;

    [[nodiscard]] auto getLocation() const -> auto const& { return m_location; }

private:
    std::source_location m_location {};
};

// NOLINTBEGIN

class ErrorHandler
{
public:
    ErrorHandler(Error const& err)
    {
#if DEBUG
        logger::logAt<logger::level::err>(err.getLocation(), "{}", err.what());
        throw err;
#else
        logger::error("{}", err.what());
        exit(-1);
#endif
    };
};

#if DEBUG
#    define MC_TRY   try
#    define MC_CATCH catch
#else
#    define MC_TRY        if constexpr (true)
#    define MC_CATCH(...) if constexpr (false)
#endif

#define MC_THROW [[maybe_unused]] ErrorHandler err =

// NOLINTEND
