#include "camera/controllers/FlyController.hpp"
#include "camera/PerspectiveCamera.hpp"
#include "events/events/InputEvents.hpp"
#include "platform/Input.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Orbital {

FlyController::FlyController(float speed)
    : m_Speed(speed)
{}

// ── Per-frame update ──────────────────────────────────────────────────────────

void FlyController::OnUpdate(float dt)
{
    if (!m_Camera) return;

    // Compute the forward/right/up vectors from yaw+pitch
    const float yawRad   = glm::radians(m_Yaw);
    const float pitchRad = glm::radians(m_Pitch);

    glm::vec3 forward;
    forward.x = std::cos(pitchRad) * std::cos(yawRad);
    forward.y = std::sin(pitchRad);
    forward.z = std::cos(pitchRad) * std::sin(yawRad);
    forward   = glm::normalize(forward);

    const glm::vec3 right = glm::normalize(glm::cross(forward, {0.0f, 1.0f, 0.0f}));
    const glm::vec3 up    = glm::normalize(glm::cross(right, forward));

    const float speed = m_Speed
        * (Input::IsKeyDown(Key::LeftShift) ? kSpeedMultiplier : 1.0f)
        * dt;

    glm::vec3 pos = m_Camera->GetPosition();

    if (Input::IsKeyDown(Key::W)) pos += forward * speed;
    if (Input::IsKeyDown(Key::S)) pos -= forward * speed;
    if (Input::IsKeyDown(Key::A)) pos -= right   * speed;
    if (Input::IsKeyDown(Key::D)) pos += right   * speed;
    if (Input::IsKeyDown(Key::E)) pos += up      * speed;
    if (Input::IsKeyDown(Key::Q)) pos -= up      * speed;

    m_Camera->SetPosition(pos);
    m_Camera->SetTarget(pos + forward);
    m_Camera->SetUp(up);
}

// ── Event dispatch ────────────────────────────────────────────────────────────

bool FlyController::OnEvent(Event& event)
{
    if (auto* e = dynamic_cast<MouseButtonPressedEvent*>(&event))
        return OnMouseButton(*e);
    if (auto* e = dynamic_cast<MouseButtonReleasedEvent*>(&event))
        return OnMouseButtonReleased(*e);
    if (auto* e = dynamic_cast<MouseMovedEvent*>(&event))
        return OnMouseMoved(*e);
    return false;
}

bool FlyController::OnMouseButton(MouseButtonPressedEvent& e)
{
    if (e.Button == 1) { // Right button
        m_IsLooking = true;
        m_LastMouse = {-1.0f, -1.0f};
    }
    return false;
}

bool FlyController::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
{
    if (e.Button == 1) m_IsLooking = false;
    return false;
}

bool FlyController::OnMouseMoved(MouseMovedEvent& e)
{
    if (!m_IsLooking) return false;

    const glm::vec2 current = {e.X, e.Y};
    if (m_LastMouse.x < 0.0f) {
        m_LastMouse = current;
        return false;
    }

    const glm::vec2 delta = current - m_LastMouse;
    m_LastMouse           = current;

    m_Yaw   += delta.x * kMouseSensitivity;
    m_Pitch -= delta.y * kMouseSensitivity; // Invert Y for natural feel
    m_Pitch  = std::clamp(m_Pitch, -89.0f, 89.0f);

    return true;
}

void FlyController::UpdateCameraDirection()
{
    // Called when yaw/pitch changes outside of OnUpdate (e.g. programmatic reset)
    if (!m_Camera) return;

    const float yawRad   = glm::radians(m_Yaw);
    const float pitchRad = glm::radians(m_Pitch);

    glm::vec3 forward;
    forward.x = std::cos(pitchRad) * std::cos(yawRad);
    forward.y = std::sin(pitchRad);
    forward.z = std::cos(pitchRad) * std::sin(yawRad);
    forward   = glm::normalize(forward);

    m_Camera->SetTarget(m_Camera->GetPosition() + forward);
}

} // namespace Orbital
