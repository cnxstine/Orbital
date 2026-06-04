#pragma once

/**
 * @file events/events/InputEvents.hpp
 * @brief Keyboard and mouse input events.
 *
 * Key codes mirror GLFW values directly so that platform/Input.hpp
 * can forward them without translation. If a future backend uses
 * a different key code system, a translation layer goes in Input.hpp.
 */

#include "events/Event.hpp"
#include <format>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Keyboard
// ─────────────────────────────────────────────────────────────────────────────

struct KeyPressedEvent : Event {
    int  KeyCode    = 0;
    int  Mods       = 0;  ///< GLFW modifier bitmask
    bool IsRepeat   = false;

    KeyPressedEvent(int key, int mods, bool repeat)
        : KeyCode(key), Mods(mods), IsRepeat(repeat) {}

    ORBITAL_EVENT_TYPE(KeyPressed)
    ORBITAL_EVENT_CATEGORY(EventCategory::Input | EventCategory::Keyboard)

    [[nodiscard]] std::string ToString() const override {
        return std::format("KeyPressedEvent: key={} mods={} repeat={}", KeyCode, Mods, IsRepeat);
    }
};

struct KeyReleasedEvent : Event {
    int KeyCode = 0;
    int Mods    = 0;

    KeyReleasedEvent(int key, int mods) : KeyCode(key), Mods(mods) {}

    ORBITAL_EVENT_TYPE(KeyReleased)
    ORBITAL_EVENT_CATEGORY(EventCategory::Input | EventCategory::Keyboard)

    [[nodiscard]] std::string ToString() const override {
        return std::format("KeyReleasedEvent: key={} mods={}", KeyCode, Mods);
    }
};

struct KeyTypedEvent : Event {
    unsigned int Codepoint = 0; ///< Unicode codepoint

    explicit KeyTypedEvent(unsigned int cp) : Codepoint(cp) {}

    ORBITAL_EVENT_TYPE(KeyTyped)
    ORBITAL_EVENT_CATEGORY(EventCategory::Input | EventCategory::Keyboard)

    [[nodiscard]] std::string ToString() const override {
        return std::format("KeyTypedEvent: U+{:04X}", Codepoint);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Mouse
// ─────────────────────────────────────────────────────────────────────────────

struct MouseButtonPressedEvent : Event {
    int Button = 0;
    int Mods   = 0;

    MouseButtonPressedEvent(int button, int mods) : Button(button), Mods(mods) {}

    ORBITAL_EVENT_TYPE(MouseButtonPressed)
    ORBITAL_EVENT_CATEGORY(EventCategory::Input | EventCategory::Mouse)

    [[nodiscard]] std::string ToString() const override {
        return std::format("MouseButtonPressedEvent: button={}", Button);
    }
};

struct MouseButtonReleasedEvent : Event {
    int Button = 0;
    int Mods   = 0;

    MouseButtonReleasedEvent(int button, int mods) : Button(button), Mods(mods) {}

    ORBITAL_EVENT_TYPE(MouseButtonReleased)
    ORBITAL_EVENT_CATEGORY(EventCategory::Input | EventCategory::Mouse)

    [[nodiscard]] std::string ToString() const override {
        return std::format("MouseButtonReleasedEvent: button={}", Button);
    }
};

struct MouseMovedEvent : Event {
    float X = 0.0f;
    float Y = 0.0f;

    MouseMovedEvent(float x, float y) : X(x), Y(y) {}

    ORBITAL_EVENT_TYPE(MouseMoved)
    ORBITAL_EVENT_CATEGORY(EventCategory::Input | EventCategory::Mouse)

    [[nodiscard]] std::string ToString() const override {
        return std::format("MouseMovedEvent: ({:.1f}, {:.1f})", X, Y);
    }
};

struct MouseScrolledEvent : Event {
    float OffsetX = 0.0f;
    float OffsetY = 0.0f;

    MouseScrolledEvent(float ox, float oy) : OffsetX(ox), OffsetY(oy) {}

    ORBITAL_EVENT_TYPE(MouseScrolled)
    ORBITAL_EVENT_CATEGORY(EventCategory::Input | EventCategory::Mouse)

    [[nodiscard]] std::string ToString() const override {
        return std::format("MouseScrolledEvent: ({:.2f}, {:.2f})", OffsetX, OffsetY);
    }
};

} // namespace Orbital
