#pragma once

/**
 * @file events/events/WindowEvents.hpp
 * @brief Window lifecycle and geometry events.
 */

#include "events/Event.hpp"
#include <format>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
struct WindowCloseEvent : Event {
    ORBITAL_EVENT_TYPE(WindowClose)
    ORBITAL_EVENT_CATEGORY(EventCategory::Window)

    [[nodiscard]] std::string ToString() const override { return "WindowCloseEvent"; }
};

// ─────────────────────────────────────────────────────────────────────────────
struct WindowResizeEvent : Event {
    uint32_t Width  = 0;
    uint32_t Height = 0;

    WindowResizeEvent(uint32_t w, uint32_t h) : Width(w), Height(h) {}

    ORBITAL_EVENT_TYPE(WindowResize)
    ORBITAL_EVENT_CATEGORY(EventCategory::Window)

    [[nodiscard]] std::string ToString() const override {
        return std::format("WindowResizeEvent: {}x{}", Width, Height);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
struct WindowFocusEvent : Event {
    bool Focused = false;

    explicit WindowFocusEvent(bool focused) : Focused(focused) {}

    ORBITAL_EVENT_TYPE(WindowFocus)
    ORBITAL_EVENT_CATEGORY(EventCategory::Window)

    [[nodiscard]] std::string ToString() const override {
        return std::format("WindowFocusEvent: {}", Focused ? "gained" : "lost");
    }
};

} // namespace Orbital
