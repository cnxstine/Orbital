#include "core/Log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

namespace Orbital {

std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

void Log::Init()
{
    // Build a shared sink list: colour console + rotating file
    std::vector<spdlog::sink_ptr> coreSinks;
    coreSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    // Try to add file sink; not fatal if it fails (read-only FS, etc.)
    try {
        coreSinks.emplace_back(
            std::make_shared<spdlog::sinks::basic_file_sink_mt>("orbital.log", /*truncate=*/true));
    } catch (const spdlog::spdlog_ex&) {
        // File logging unavailable — continue with console only
    }

    s_CoreLogger = std::make_shared<spdlog::logger>("ORBITAL", coreSinks.begin(), coreSinks.end());
    s_CoreLogger->set_level(spdlog::level::trace);
    s_CoreLogger->set_pattern("[%T.%e] [%n] [%^%l%$] %v");
    spdlog::register_logger(s_CoreLogger);

    // Client logger re-uses the same sinks for now
    s_ClientLogger = std::make_shared<spdlog::logger>("APP", coreSinks.begin(), coreSinks.end());
    s_ClientLogger->set_level(spdlog::level::trace);
    s_ClientLogger->set_pattern("[%T.%e] [%n] [%^%l%$] %v");
    spdlog::register_logger(s_ClientLogger);

    s_CoreLogger->info("Logging initialised");
}

std::shared_ptr<spdlog::logger>& Log::GetCoreLogger() noexcept
{
    return s_CoreLogger;
}

std::shared_ptr<spdlog::logger>& Log::GetClientLogger() noexcept
{
    return s_ClientLogger;
}

} // namespace Orbital
