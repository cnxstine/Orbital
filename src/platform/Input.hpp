#pragma once

/**
 * @file platform/Input.hpp
 * @brief Stateless input query API — poll key/button/mouse state each frame.
 *
 * Design: static query functions backed by a cached GLFWwindow*.
 * Updated once per frame via Input::Update(GLFWwindow*) before event dispatch.
 *
 * Key codes are raw GLFW key constants (e.g. GLFW_KEY_W = 87).
 * Include `<GLFW/glfw3.h>` in calling code only when using GLFW_ constants,
 * or use the named Key:: aliases defined in this header.
 */

#include <glm/glm.hpp>

// Forward-declare to avoid including GLFW in headers
struct GLFWwindow;

namespace Orbital {

/// GLFW key code aliases — avoids GLFW include in most files.
namespace Key {
    inline constexpr int Space     = 32;
    inline constexpr int Apostrophe= 39;
    inline constexpr int Comma     = 44;
    inline constexpr int Minus     = 45;
    inline constexpr int Period    = 46;
    inline constexpr int Slash     = 47;

    inline constexpr int D0 = 48, D1 = 49, D2 = 50, D3 = 51, D4 = 52;
    inline constexpr int D5 = 53, D6 = 54, D7 = 55, D8 = 56, D9 = 57;

    inline constexpr int A = 65, B = 66, C = 67, D = 68, E = 69;
    inline constexpr int F = 70, G = 71, H = 72, I = 73, J = 74;
    inline constexpr int K = 75, L = 76, M = 77, N = 78, O = 79;
    inline constexpr int P = 80, Q = 81, R = 82, S = 83, T = 84;
    inline constexpr int U = 85, V = 86, W = 87, X = 88, Y = 89;
    inline constexpr int Z = 90;

    inline constexpr int Escape    = 256;
    inline constexpr int Enter     = 257;
    inline constexpr int Tab       = 258;
    inline constexpr int Backspace = 259;
    inline constexpr int Insert    = 260;
    inline constexpr int Delete    = 261;
    inline constexpr int Right     = 262;
    inline constexpr int Left      = 263;
    inline constexpr int Down      = 264;
    inline constexpr int Up        = 265;

    inline constexpr int F1  = 290, F2  = 291, F3  = 292, F4  = 293;
    inline constexpr int F5  = 294, F6  = 295, F7  = 296, F8  = 297;
    inline constexpr int F9  = 298, F10 = 299, F11 = 300, F12 = 301;

    inline constexpr int LeftShift   = 340;
    inline constexpr int LeftControl = 341;
    inline constexpr int LeftAlt     = 342;
    inline constexpr int RightShift  = 344;
    inline constexpr int RightControl= 345;
    inline constexpr int RightAlt    = 346;
} // namespace Key

namespace MouseButton {
    inline constexpr int Left   = 0;
    inline constexpr int Right  = 1;
    inline constexpr int Middle = 2;
} // namespace MouseButton

// ─────────────────────────────────────────────────────────────────────────────

class Input {
public:
    Input() = delete;

    /// Must be called once per frame (before event dispatch) with the active window.
    static void Update(GLFWwindow* window);

    // ── Keyboard ──────────────────────────────────────────────────────────────

    /// Returns true while the key is held down.
    [[nodiscard]] static bool IsKeyDown(int keyCode);

    /// Returns true only during the frame the key was first pressed.
    [[nodiscard]] static bool IsKeyPressed(int keyCode);

    /// Returns true only during the frame the key was released.
    [[nodiscard]] static bool IsKeyReleased(int keyCode);

    // ── Mouse ─────────────────────────────────────────────────────────────────

    [[nodiscard]] static bool        IsMouseButtonDown(int button);
    [[nodiscard]] static bool        IsMouseButtonPressed(int button);
    [[nodiscard]] static bool        IsMouseButtonReleased(int button);

    /// Screen-space cursor position (pixels, origin top-left).
    [[nodiscard]] static glm::vec2   GetMousePosition();

    /// Delta since the previous frame.
    [[nodiscard]] static glm::vec2   GetMouseDelta();

private:
    static GLFWwindow* s_Window;

    // Current and previous frame key state (indexed by GLFW key code 0..348)
    static constexpr int kMaxKeys    = 512;
    static constexpr int kMaxButtons = 8;

    static bool s_KeyCurrent  [kMaxKeys];
    static bool s_KeyPrevious [kMaxKeys];
    static bool s_BtnCurrent  [kMaxButtons];
    static bool s_BtnPrevious [kMaxButtons];

    static glm::vec2 s_MousePos;
    static glm::vec2 s_MousePosPrev;
};

} // namespace Orbital
