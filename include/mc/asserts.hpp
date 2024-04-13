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

#    if DEBUG
#        define KASSERT_DEBUG(expr)                                          \
            {                                                                \
                if (expr)                                                    \
                {                                                            \
                }                                                            \
                else                                                         \
                {                                                            \
                    report_assertion_failure(#expr, "", __FILE__, __LINE__); \
                    debugBreak();                                            \
                }                                                            \
            }
#    else
#        define KASSERT_DEBUG(expr)
#    endif

#else
#    define KASSERT(expr)
#    define KASSERT_MSG(expr, message)
#    define KASSERT_DEBUG(expr)
#endif
