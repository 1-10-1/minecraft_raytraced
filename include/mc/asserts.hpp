#pragma once

#include "logger.hpp"

#define KASSERTIONS_ENABLED

#ifdef KASSERTIONS_ENABLED
#    if _MSC_VER
#        include <intrin.h>
#        define debugBreak() __debugbreak()
#    else
#        define debugBreak() __builtin_trap()
#    endif

#    define KV_ASSERT(expr)                                          \
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

#    define KV_ASSERT(expr, message)                                           \
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
#        define KASSERT_DEBUG(expr)  // Does nothing at all
#    endif

#else
#    define KASSERT(expr)               // Does nothing at all
#    define KASSERT_MSG(expr, message)  // Does nothing at all
#    define KASSERT_DEBUG(expr)         // Does nothing at all
#endif
