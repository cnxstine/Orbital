#pragma once

/**
 * @file events/Event.hpp
 * @brief Base event type and category bitmask used throughout Orbital.
 *
 * Events are plain data structures — no virtual dispatch, no heap allocation
 * (when used with the inline EventBus queue).
 *
 * Each concrete event must:
 *   1. Inherit from Event
 *   2. Define static constexpr EventType  TYPE
 *   3. Define static constexpr EventCategory CATEGORY (bitmask)
 *
 * The EventBus uses TYPE for typed dispatch and CATEGORY for filtering.
 */

#include <cstdint>
#include <string>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// EventType — unique tag per event struct.
// Add new types here when you add new event headers.
// ─────────────────────────────────────────────────────────────────────────────
enum class EventType : uint32_t {
    None = 0,

    // Window
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,

    // Input — keyboard
    KeyPressed,
    KeyReleased,
    KeyTyped,

    // Input — mouse
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,

    // Scene
    SceneLoaded,
    CameraChanged,

    // Simulation (reserved for later)
    SimulationStepCompleted,
    SimulationParametersChanged,
};

// ─────────────────────────────────────────────────────────────────────────────
// EventCategory — bitmask for coarse filtering.
// ─────────────────────────────────────────────────────────────────────────────
enum class EventCategory : uint32_t {
    None       = 0,
    Window     = 1u << 0,
    Input      = 1u << 1,
    Keyboard   = 1u << 2,
    Mouse      = 1u << 3,
    Scene      = 1u << 4,
    Simulation = 1u << 5,
    UI         = 1u << 6,
};

constexpr EventCategory operator|(EventCategory a, EventCategory b) noexcept {
    return static_cast<EventCategory>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
constexpr bool HasCategory(EventCategory mask, EventCategory bit) noexcept {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(bit)) != 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Event base — all concrete events inherit this.
// ─────────────────────────────────────────────────────────────────────────────
struct Event {
    bool Handled = false; ///< Set to true to stop propagation.

    [[nodiscard]] virtual EventType     GetType()       const noexcept = 0;
    [[nodiscard]] virtual EventCategory GetCategory()   const noexcept = 0;
    [[nodiscard]] virtual std::string   ToString()      const          = 0;

    [[nodiscard]] bool IsInCategory(EventCategory cat) const noexcept {
        return HasCategory(GetCategory(), cat);
    }

    virtual ~Event() = default;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helper macro — reduces boilerplate in concrete event structs.
// ─────────────────────────────────────────────────────────────────────────────
#define ORBITAL_EVENT_TYPE(type)                                                  \
    static  constexpr EventType StaticType() noexcept { return EventType::type; } \
    [[nodiscard]] EventType     GetType()    const noexcept override               \
        { return EventType::type; }

#define ORBITAL_EVENT_CATEGORY(cat)                                               \
    [[nodiscard]] EventCategory GetCategory() const noexcept override             \
        { return cat; }

} // namespace Orbital
