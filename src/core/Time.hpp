#pragma once

/**
 * @file core/Time.hpp
 * @brief Frame timing: variable delta-time and fixed-step accumulator.
 *
 * The engine uses two time modes:
 *
 *  Variable dt  – passed to Layer::OnUpdate(dt). Used for animations,
 *                 camera smoothing, and UI. Can vary arbitrarily per frame.
 *
 *  Fixed dt     – passed to Layer::OnFixedUpdate(fixedDt). Always the same
 *                 value, determined by the configured tick rate. Simulation
 *                 code uses this for determinism.
 *
 * Usage:
 *   // Each frame:
 *   Time::Tick();
 *   float dt = Time::Delta();
 *
 *   // Fixed-step accumulator loop:
 *   m_Accumulator += Time::Delta();
 *   while (m_Accumulator >= Time::Fixed()) {
 *       layer.OnFixedUpdate(Time::Fixed());
 *       m_Accumulator -= Time::Fixed();
 *   }
 */

#include <chrono>

namespace Orbital {

class Time {
public:
    Time() = delete;

    // ── Configuration ─────────────────────────────────────────────────────────

    /// Set the fixed simulation timestep (default: 1/60 s).
    static void SetFixedTimestep(float seconds) noexcept;

    // ── Per-frame update ──────────────────────────────────────────────────────

    /// Must be called exactly once at the start of each frame.
    static void Tick() noexcept;

    // ── Accessors ─────────────────────────────────────────────────────────────

    /// Variable delta-time of the last frame in seconds.
    [[nodiscard]] static float Delta()    noexcept { return s_DeltaTime;  }

    /// Configured fixed simulation timestep in seconds.
    [[nodiscard]] static float Fixed()    noexcept { return s_FixedStep;  }

    /// Elapsed time since application startup in seconds.
    [[nodiscard]] static float Elapsed()  noexcept { return s_Elapsed;    }

    /// Frames per second (1 / DeltaTime, smoothed over 60 frames).
    [[nodiscard]] static float FPS()      noexcept { return s_FPS;        }

    /// Frame counter (incremented each Tick call).
    [[nodiscard]] static uint64_t Frame() noexcept { return s_FrameCount; }

private:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    static TimePoint  s_LastFrame;
    static TimePoint  s_StartTime;

    static float    s_DeltaTime;
    static float    s_Elapsed;
    static float    s_FixedStep;
    static float    s_FPS;
    static uint64_t s_FrameCount;
};

} // namespace Orbital
