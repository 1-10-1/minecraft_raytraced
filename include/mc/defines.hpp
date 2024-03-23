#pragma once

#ifdef NDEBUG
constexpr bool kDebug = false;
#else
constexpr bool kDebug = true;
#endif

#define ALWAYS_INLINE  // TODO(aether): Do something about this
#define NEVER_INLINE [[clang::noinline]]
