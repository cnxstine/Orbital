#pragma once

/**
 * @file core/Log.hpp
 * @brief Structured logging facade wrapping spdlog.
 *
 * Two named loggers are created at startup:
 *  - "ORBITAL"  → engine-internal messages (ORB_CORE_*)
 *  - "APP"      → user/module-level messages (ORB_*)
 *
 * Both write to the console with colour coding and timestamps.
 * In Release builds the trace level is compiled out via NDEBUG.
 *
 * Usage (engine code):
 *   ORB_CORE_INFO("Renderer initialised in {}ms", elapsed);
 *
 * Usage (module/app code):
 *   ORB_INFO("Module '{}' attached", name);
 */

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <memory>

namespace Orbital {

/// Central logging facility. Call Log::Init() once before any ORB_* macros.
class Log {
public:
    Log() = delete;

    /// Initialise both named loggers. Must be the first call in Application::Run().
    static void Init();

    /// Engine-internal logger (ORB_CORE_* macros).
    [[nodiscard]] static std::shared_ptr<spdlog::logger>& GetCoreLogger() noexcept;

    /// Client / module logger (ORB_* macros).
    [[nodiscard]] static std::shared_ptr<spdlog::logger>& GetClientLogger() noexcept;

private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

} // namespace Orbital

// ── Engine-internal macros ────────────────────────────────────────────────────
#define ORB_CORE_TRACE(...)    ::Orbital::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ORB_CORE_DEBUG(...)    ::Orbital::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define ORB_CORE_INFO(...)     ::Orbital::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ORB_CORE_WARN(...)     ::Orbital::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ORB_CORE_ERROR(...)    ::Orbital::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ORB_CORE_CRITICAL(...) ::Orbital::Log::GetCoreLogger()->critical(__VA_ARGS__)

// ── Application / module macros ───────────────────────────────────────────────
#define ORB_TRACE(...)    ::Orbital::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ORB_DEBUG(...)    ::Orbital::Log::GetClientLogger()->debug(__VA_ARGS__)
#define ORB_INFO(...)     ::Orbital::Log::GetClientLogger()->info(__VA_ARGS__)
#define ORB_WARN(...)     ::Orbital::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ORB_ERROR(...)    ::Orbital::Log::GetClientLogger()->error(__VA_ARGS__)
#define ORB_CRITICAL(...) ::Orbital::Log::GetClientLogger()->critical(__VA_ARGS__)
