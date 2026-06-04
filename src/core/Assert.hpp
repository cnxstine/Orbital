#pragma once

/**
 * @file core/Assert.hpp
 * @brief Assertion macros that log context and abort cleanly.
 *
 * Debug assertions are stripped in Release builds (when NDEBUG is defined).
 *
 * ORB_ASSERT(condition, ...)     – engine-internal assertion
 * ORB_VERIFY(condition, ...)     – always-evaluated; only aborts in Debug
 *
 * Example:
 *   ORB_ASSERT(shader != nullptr, "Null shader passed to Renderer::Submit");
 */

#include "core/Log.hpp"
#include <source_location>
#include <cstdlib>

namespace Orbital::detail {

[[noreturn]] inline void AssertFail(
    const char*              expr,
    const char*              msg,
    std::source_location     loc = std::source_location::current()) noexcept
{
    ORB_CORE_CRITICAL("Assertion failed: {}\n  Expression : {}\n  File       : {}:{}\n  Function   : {}",
        msg, expr, loc.file_name(), loc.line(), loc.function_name());

    // Flush before aborting so log output is not lost
    spdlog::shutdown();
    std::abort();
}

} // namespace Orbital::detail

#ifndef NDEBUG
    /// Assert that fires only in Debug builds.
    #define ORB_ASSERT(expr, ...) \
        do { \
            if (!(expr)) { \
                ::Orbital::detail::AssertFail(#expr, "" __VA_ARGS__); \
            } \
        } while(false)
#else
    #define ORB_ASSERT(expr, ...) ((void)(expr))
#endif

/// Always evaluated; aborts in Debug, logs in Release.
#define ORB_VERIFY(expr, ...) \
    do { \
        if (!(expr)) { \
            ORB_CORE_ERROR("Verify failed: {}", "" __VA_ARGS__); \
            ORB_ASSERT(false, "" __VA_ARGS__); \
        } \
    } while(false)
