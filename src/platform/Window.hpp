#pragma once

/**
 * @file platform/Window.hpp
 * @brief GLFW window and OpenGL context wrapper.
 *
 * Window owns the GLFW window handle and GL context lifetime.
 * It creates the context, installs GLFW callbacks that route into the
 * EventBus, and exposes a minimal surface API.
 *
 * Design decisions:
 *  - The Window installs callbacks that forward to an EventBus reference.
 *    This decouples GLFW from all other engine code.
 *  - No global state: GLFW user pointer points to the owning Window.
 *  - Exactly one Window per process; GLFW does not support multiple contexts
 *    well without explicit care (not needed for Orbital).
 */

#include "events/EventBus.hpp"

#include <string>
#include <cstdint>

// Forward declarations to avoid including GLFW in headers
struct GLFWwindow;

namespace Orbital {

struct WindowSpec {
    std::string  Title  = "Orbital";
    uint32_t     Width  = 1600;
    uint32_t     Height = 900;
    bool         VSync  = true;
};

class Window {
public:
    explicit Window(const WindowSpec& spec, EventBus& bus);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    // ── Frame interface ────────────────────────────────────────────────────────

    /// Poll OS events (fills EventBus queue).
    void PollEvents() const;

    /// Swap the back buffer to the screen.
    void SwapBuffers() const;

    // ── Queries ───────────────────────────────────────────────────────────────

    [[nodiscard]] uint32_t     GetWidth()  const noexcept { return m_Spec.Width;  }
    [[nodiscard]] uint32_t     GetHeight() const noexcept { return m_Spec.Height; }
    [[nodiscard]] float        GetAspect() const noexcept {
        return static_cast<float>(m_Spec.Width) / static_cast<float>(m_Spec.Height);
    }
    [[nodiscard]] bool         ShouldClose()   const;
    [[nodiscard]] GLFWwindow*  GetNativeHandle() const noexcept { return m_Handle; }

    // ── VSync ─────────────────────────────────────────────────────────────────
    void SetVSync(bool enabled);
    [[nodiscard]] bool IsVSync() const noexcept { return m_Spec.VSync; }

private:
    // ── GLFW static callbacks (routed to the Window via user pointer) ──────────
    static void CB_WindowClose    (GLFWwindow*);
    static void CB_WindowResize   (GLFWwindow*, int w, int h);
    static void CB_WindowFocus    (GLFWwindow*, int focused);
    static void CB_KeyCallback    (GLFWwindow*, int key, int scancode, int action, int mods);
    static void CB_CharCallback   (GLFWwindow*, unsigned int codepoint);
    static void CB_MouseButton    (GLFWwindow*, int button, int action, int mods);
    static void CB_CursorPos      (GLFWwindow*, double x, double y);
    static void CB_Scroll         (GLFWwindow*, double ox, double oy);

    void InstallCallbacks();

    WindowSpec  m_Spec;
    GLFWwindow* m_Handle = nullptr;
    EventBus&   m_Bus;
};

} // namespace Orbital
