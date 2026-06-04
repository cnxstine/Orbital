#include "camera/controllers/ArcballController.hpp"
#include "camera/PerspectiveCamera.hpp"
#include "events/events/InputEvents.hpp"

#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace Orbital {

ArcballController::ArcballController()
{
    // Start with camera above and in front of origin
    UpdateCameraTransform();
}

void ArcballController::SetCamera(PerspectiveCamera* camera)
{
    CameraController::SetCamera(camera);
    UpdateCameraTransform();
}

void ArcballController::SetViewPoint(const glm::vec3& pivot, float radius, float theta, float phi) noexcept
{
    m_Pivot = pivot;
    m_Radius = glm::max(radius, m_MinRadius);
    m_Theta = theta;
    m_Phi = std::clamp(phi, -kMaxPhi, kMaxPhi);
    UpdateCameraTransform();
}

// ── Per-frame update ──────────────────────────────────────────────────────────

void ArcballController::OnUpdate(float /*dt*/)
{
    // Camera transform is updated reactively in OnEvent handlers.
    // No continuous integration needed for pure arcball.
    // (Future: smooth interpolation / inertia would go here)
}

// ── Event dispatch ────────────────────────────────────────────────────────────

bool ArcballController::OnEvent(Event& event)
{
    if (auto* e = dynamic_cast<MouseButtonPressedEvent*>(&event))
        return OnMouseButton(*e);
    if (auto* e = dynamic_cast<MouseButtonReleasedEvent*>(&event))
        return OnMouseButtonReleased(*e);
    if (auto* e = dynamic_cast<MouseMovedEvent*>(&event))
        return OnMouseMoved(*e);
    if (auto* e = dynamic_cast<MouseScrolledEvent*>(&event))
        return OnMouseScrolled(*e);
    return false;
}

// ── Event handlers ────────────────────────────────────────────────────────────

bool ArcballController::OnMouseButton(MouseButtonPressedEvent& e)
{
    constexpr int kLeftButton  = 0; // GLFW_MOUSE_BUTTON_LEFT
    constexpr int kRightButton = 1; // GLFW_MOUSE_BUTTON_RIGHT

    if (e.Button == kLeftButton) {
        m_IsRotating = true;
        m_LastMouse  = {-1.0f, -1.0f}; // Sentinel — first move sets baseline
        return false; // Don't consume: UI panels also need to know
    }
    if (e.Button == kRightButton) {
        m_IsPanning = true;
        m_LastMouse = {-1.0f, -1.0f};
        return false;
    }
    return false;
}

bool ArcballController::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
{
    if (e.Button == 0) m_IsRotating = false;
    if (e.Button == 1) m_IsPanning  = false;
    return false;
}

bool ArcballController::OnMouseMoved(MouseMovedEvent& e)
{
    const glm::vec2 current = {e.X, e.Y};

    // First move after button press: just record position, don't apply delta
    if (m_LastMouse.x < 0.0f && m_LastMouse.y < 0.0f) {
        m_LastMouse = current;
        return false;
    }

    const glm::vec2 delta = current - m_LastMouse;
    m_LastMouse           = current;

    if (!m_Camera) return false;

    if (m_IsRotating) {
        // Horizontal mouse → azimuth; vertical mouse → elevation
        m_Theta += delta.x * kRotateSensitivity;
        m_Phi   -= delta.y * kRotateSensitivity; // Inverted Y: up = decrease phi
        m_Phi    = std::clamp(m_Phi, -kMaxPhi, kMaxPhi);
        UpdateCameraTransform();
        return true;
    }

    if (m_IsPanning) {
        // Pan in the view plane: move both pivot and camera
        const glm::vec3 right = m_Camera->GetRight();
        const glm::vec3 up    = m_Camera->GetUp();

        const float panScale = m_Radius * kPanSensitivity;
        const glm::vec3 offset = (-delta.x * right + delta.y * up) * panScale;

        m_Pivot += offset;
        UpdateCameraTransform();
        return true;
    }

    return false;
}

bool ArcballController::OnMouseScrolled(MouseScrolledEvent& e)
{
    if (!m_Camera) return false;

    // Dolly: move radius, keep pivot
    m_Radius -= e.OffsetY * kScrollSensitivity * m_Radius; // Scale by current radius for feel
    m_Radius  = std::max(m_Radius, m_MinRadius);
    UpdateCameraTransform();
    return true;
}

// ── Spherical → Cartesian ─────────────────────────────────────────────────────

void ArcballController::UpdateCameraTransform()
{
    if (!m_Camera) return;

    // Spherical to Cartesian (Y-up)
    const float x = m_Radius * std::sin(m_Phi) * std::cos(m_Theta);
    const float y = m_Radius * std::cos(m_Phi);
    const float z = m_Radius * std::sin(m_Phi) * std::sin(m_Theta);

    const glm::vec3 position = m_Pivot + glm::vec3{x, y, z};

    m_Camera->SetPosition(position);
    m_Camera->SetTarget(m_Pivot);
    m_Camera->SetUp({0.0f, 1.0f, 0.0f});
}

} // namespace Orbital
