#pragma once

#ifdef NDEBUG
constexpr bool kDebug = false;
#else
constexpr bool kDebug = true;
#endif

#if PROFILED
constexpr bool kProfiled = true;
#else
constexpr bool kProfiled = false;
#endif

#define PROFILED_BLOCK if constexpr (kProfiled)

#define ALWAYS_INLINE  // TODO(aether): Do something about this
#define NEVER_INLINE [[clang::noinline]]
