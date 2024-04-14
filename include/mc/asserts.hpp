#pragma once

#include "logger.hpp"

#if _MSC_VER
#    include <intrin.h>
#    define debugBreak() __debugbreak()
#else
#    define debugBreak() __builtin_trap()
#endif

#ifdef ASSERTIONS_ENABLED
#    define MC_ASSERT(expr)                                          \
        {                                                            \
            if (expr)                                                \
            {                                                        \
            }                                                        \
            else                                                     \
            {                                                        \
                logger::critical("Assertion '{}' failed.", / #expr); \
                debugBreak();                                        \
            }                                                        \
        }

#    define MC_ASSERT_MSG(expr, message)                                       \
        {                                                                      \
            if (expr)                                                          \
            {                                                                  \
            }                                                                  \
            else                                                               \
            {                                                                  \
                logger::critical("Assertion '{}' failed: {}", #expr, message); \
                debugBreak();                                                  \
            }                                                                  \
        }
#else
#    define MC_ASSERT(expr)
#    define MC_ASSERT_MSG(expr, message)
#endif
