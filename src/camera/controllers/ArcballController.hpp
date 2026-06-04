#pragma once

/**
 * @file camera/controllers/ArcballController.hpp
 * @brief Arcball camera controller — primary navigation mode for Orbital.
 *
 * Behavior:
 *   Left mouse drag   → rotate around pivot (spherical coordinates)
 *   Right mouse drag  → pan camera + pivot in the view plane
 *   Scroll wheel      → dolly (move toward/away from pivot along view axis)
 *   Double-click L    → reset to default orientation (future)
 *
 * Implementation uses spherical coordinates (Theta, Phi, Radius) relative
 * to a pivot point. Each frame the camera position is computed as:
 *   pos = pivot + radius * (sin(phi)*cos(theta), cos(phi), sin(phi)*sin(theta))
 *
 * Phi is elevation (clamped ±89° to avoid gimbal at poles).
 * Theta is azimuth.
 */

#include "camera/CameraController.hpp"
#include <glm/glm.hpp>

namespace Orbital {

class ArcballController final : public CameraController {
public:
    ArcballController();
    ~ArcballController() override = default;

    void OnUpdate(float dt) override;
    bool OnEvent(Event& event) override;

    void SetCamera(PerspectiveCamera* camera) override;

    // ── Configuration ─────────────────────────────────────────────────────────
    void SetPivot(const glm::vec3& pivot)    noexcept { m_Pivot  = pivot;  }
    void SetRadius(float radius)             noexcept { m_Radius = glm::max(radius, m_MinRadius); }
    void SetViewPoint(const glm::vec3& pivot, float radius, float theta, float phi) noexcept;

    [[nodiscard]] glm::vec3 GetPivot()  const noexcept { return m_Pivot;  }
    [[nodiscard]] float     GetRadius() const noexcept { return m_Radius; }
    [[nodiscard]] float     GetTheta()  const noexcept { return m_Theta;  }
    [[nodiscard]] float     GetPhi()    const noexcept { return m_Phi;    }

private:
    bool OnMouseButton(struct MouseButtonPressedEvent&  e);
    bool OnMouseButtonReleased(struct MouseButtonReleasedEvent& e);
    bool OnMouseMoved(struct MouseMovedEvent& e);
    bool OnMouseScrolled(struct MouseScrolledEvent& e);

    void UpdateCameraTransform();

    // ── Spherical state ───────────────────────────────────────────────────────
    glm::vec3 m_Pivot  = {0.0f, 0.0f, 0.0f};
    float     m_Radius = 5.0f;
    float     m_Theta  = 0.0f;                     ///< Azimuth (radians)
    float     m_Phi    = glm::radians(30.0f);       ///< Elevation (radians)

    static constexpr float kMinRadius   = 0.05f;
    static constexpr float kMaxPhi      = glm::radians(89.0f);
    float m_MinRadius = kMinRadius;

    // ── Drag state ────────────────────────────────────────────────────────────
    bool      m_IsRotating = false;
    bool      m_IsPanning  = false;
    glm::vec2 m_LastMouse  = {0.0f, 0.0f};

    // ── Sensitivity constants ─────────────────────────────────────────────────
    static constexpr float kRotateSensitivity = 0.005f;
    static constexpr float kPanSensitivity    = 0.003f;
    static constexpr float kScrollSensitivity = 0.25f;
};

} // namespace Orbital
