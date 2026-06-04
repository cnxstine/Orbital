#include "core/Time.hpp"

#include <algorithm>
#include <array>
#include <numeric>

namespace Orbital {

// ── Static storage ────────────────────────────────────────────────────────────
Time::TimePoint  Time::s_LastFrame  = Time::Clock::now();
Time::TimePoint  Time::s_StartTime  = Time::Clock::now();
float            Time::s_DeltaTime  = 0.0f;
float            Time::s_Elapsed    = 0.0f;
float            Time::s_FixedStep  = 1.0f / 60.0f;
float            Time::s_FPS        = 0.0f;
uint64_t         Time::s_FrameCount = 0;

// ── Implementation ────────────────────────────────────────────────────────────

void Time::SetFixedTimestep(float seconds) noexcept
{
    s_FixedStep = std::max(seconds, 0.0001f); // Guard against division by zero
}

void Time::Tick() noexcept
{
    const auto now = Clock::now();

    // Variable delta — clamped to avoid spiral-of-death on focus loss / debug
    const float raw = std::chrono::duration<float>(now - s_LastFrame).count();
    s_DeltaTime = std::min(raw, 0.25f); // Max 250ms per frame

    s_Elapsed   = std::chrono::duration<float>(now - s_StartTime).count();
    s_LastFrame = now;
    ++s_FrameCount;

    // Exponential moving average for FPS (avoids per-frame spikes in display)
    constexpr float kAlpha = 0.05f;
    const float     instantFPS = (s_DeltaTime > 0.0f) ? (1.0f / s_DeltaTime) : 0.0f;
    s_FPS = (s_FrameCount == 1)
        ? instantFPS
        : s_FPS + kAlpha * (instantFPS - s_FPS);
}

} // namespace Orbital
