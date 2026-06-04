#include "platform/Input.hpp"

#include <GLFW/glfw3.h>
#include <cstring>

namespace Orbital {

// ── Static storage ────────────────────────────────────────────────────────────
GLFWwindow* Input::s_Window = nullptr;

bool Input::s_KeyCurrent  [Input::kMaxKeys]    = {};
bool Input::s_KeyPrevious [Input::kMaxKeys]    = {};
bool Input::s_BtnCurrent  [Input::kMaxButtons] = {};
bool Input::s_BtnPrevious [Input::kMaxButtons] = {};

glm::vec2 Input::s_MousePos     = {};
glm::vec2 Input::s_MousePosPrev = {};

// ── Per-frame update ──────────────────────────────────────────────────────────

void Input::Update(GLFWwindow* window)
{
    s_Window = window;

    // Snapshot current → previous
    std::memcpy(s_KeyPrevious, s_KeyCurrent, sizeof(s_KeyCurrent));
    std::memcpy(s_BtnPrevious, s_BtnCurrent, sizeof(s_BtnCurrent));
    s_MousePosPrev = s_MousePos;

    // Sample keyboard state
    for (int i = 0; i < kMaxKeys; ++i) {
        int state = glfwGetKey(window, i);
        s_KeyCurrent[i] = (state == GLFW_PRESS || state == GLFW_REPEAT);
    }

    // Sample mouse button state
    for (int i = 0; i < kMaxButtons; ++i) {
        s_BtnCurrent[i] = (glfwGetMouseButton(window, i) == GLFW_PRESS);
    }

    // Sample cursor position
    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(window, &mx, &my);
    s_MousePos = { static_cast<float>(mx), static_cast<float>(my) };
}

// ── Keyboard ──────────────────────────────────────────────────────────────────

bool Input::IsKeyDown(int keyCode)
{
    if (keyCode < 0 || keyCode >= kMaxKeys) return false;
    return s_KeyCurrent[keyCode];
}

bool Input::IsKeyPressed(int keyCode)
{
    if (keyCode < 0 || keyCode >= kMaxKeys) return false;
    return s_KeyCurrent[keyCode] && !s_KeyPrevious[keyCode];
}

bool Input::IsKeyReleased(int keyCode)
{
    if (keyCode < 0 || keyCode >= kMaxKeys) return false;
    return !s_KeyCurrent[keyCode] && s_KeyPrevious[keyCode];
}

// ── Mouse ─────────────────────────────────────────────────────────────────────

bool Input::IsMouseButtonDown(int button)
{
    if (button < 0 || button >= kMaxButtons) return false;
    return s_BtnCurrent[button];
}

bool Input::IsMouseButtonPressed(int button)
{
    if (button < 0 || button >= kMaxButtons) return false;
    return s_BtnCurrent[button] && !s_BtnPrevious[button];
}

bool Input::IsMouseButtonReleased(int button)
{
    if (button < 0 || button >= kMaxButtons) return false;
    return !s_BtnCurrent[button] && s_BtnPrevious[button];
}

glm::vec2 Input::GetMousePosition()
{
    return s_MousePos;
}

glm::vec2 Input::GetMouseDelta()
{
    return s_MousePos - s_MousePosPrev;
}

} // namespace Orbital
