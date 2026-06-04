#pragma once

/**
 * @file camera/controllers/FlyController.hpp
 * @brief Free-fly camera controller for unrestricted 3D exploration.
 *
 * Controls:
 *   W / S          → move forward / backward
 *   A / D          → strafe left / right
 *   Q / E          → move down / up (world Y)
 *   Right mouse    → rotate (yaw / pitch)
 *   Left Shift     → speed boost (3×)
 *
 * The FlyController maintains its own yaw/pitch state rather than deriving
 * them from the camera's current direction, preventing accumulation of
 * floating-point drift over long sessions.
 */

#include "camera/CameraController.hpp"
#include <glm/glm.hpp>

namespace Orbital {

class FlyController final : public CameraController {
public:
    explicit FlyController(float speed = 3.0f);
    ~FlyController() override = default;

    void OnUpdate(float dt) override;
    bool OnEvent(Event& event)  override;

    // ── Configuration ─────────────────────────────────────────────────────────
    void  SetSpeed(float s) noexcept { m_Speed = s; }
    [[nodiscard]] float GetSpeed() const noexcept { return m_Speed; }

private:
    bool OnMouseButton(struct MouseButtonPressedEvent& e);
    bool OnMouseButtonReleased(struct MouseButtonReleasedEvent& e);
    bool OnMouseMoved(struct MouseMovedEvent& e);

    void UpdateCameraDirection();

    float     m_Speed       = 3.0f;
    float     m_Yaw         = -90.0f; ///< Degrees; -90 = looking down -Z
    float     m_Pitch       = 0.0f;   ///< Degrees; clamped ±89°

    bool      m_IsLooking   = false;
    glm::vec2 m_LastMouse   = {0.0f, 0.0f};

    static constexpr float kMouseSensitivity = 0.10f;
    static constexpr float kSpeedMultiplier  = 3.0f;
};

} // namespace Orbital
