#pragma once

/**
 * @file camera/Camera.hpp
 * @brief Abstract camera interface — view/projection matrices + geometry accessors.
 *
 * Separating Camera (data) from CameraController (input behavior) means:
 *   - The renderer reads only Camera (no controller dependency in render code).
 *   - Controllers can be hot-swapped without touching the camera matrices.
 *   - CinematicController can animate the Camera directly, bypassing controllers.
 *
 * All matrix conventions:
 *   - Column-major (GLM default, matches GLSL layout)
 *   - Right-handed coordinate system
 *   - Depth range: [0, 1] (GLM_FORCE_DEPTH_ZERO_TO_ONE enabled in CMake)
 */

#include <glm/glm.hpp>

namespace Orbital {

class Camera {
public:
    virtual ~Camera() = default;

    // ── Matrix accessors (computed lazily in derived classes) ─────────────────

    [[nodiscard]] virtual glm::mat4 GetViewMatrix()       const = 0;
    [[nodiscard]] virtual glm::mat4 GetProjectionMatrix() const = 0;

    // ── Geometry accessors ─────────────────────────────────────────────────────

    [[nodiscard]] virtual glm::vec3 GetPosition() const = 0;
    [[nodiscard]] virtual glm::vec3 GetForward()  const = 0;
    [[nodiscard]] virtual glm::vec3 GetRight()    const = 0;
    [[nodiscard]] virtual glm::vec3 GetUp()       const = 0;
    [[nodiscard]] virtual float     GetNearPlane()const = 0;
    [[nodiscard]] virtual float     GetFarPlane() const = 0;

    // ── Derived (non-virtual — just combines view * proj) ─────────────────────

    [[nodiscard]] glm::mat4 GetViewProjection() const
    {
        return GetProjectionMatrix() * GetViewMatrix();
    }
};

} // namespace Orbital
