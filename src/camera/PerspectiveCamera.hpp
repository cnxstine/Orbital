#pragma once

/**
 * @file camera/PerspectiveCamera.hpp
 * @brief Perspective camera with look-at target.
 *
 * State is stored as Position + Target + Up, not as Euler angles.
 * This avoids gimbal lock and is a natural fit for the Arcball controller,
 * which operates in spherical coordinates around a pivot.
 *
 * Matrices are recomputed lazily: only when IsDirty() is true.
 */

#include "camera/Camera.hpp"
#include <glm/glm.hpp>

namespace Orbital {

class PerspectiveCamera final : public Camera {
public:
    /**
     * @param fovRadians  Vertical field of view in radians.
     * @param aspect      Width / Height.
     * @param near        Near clip plane distance (> 0).
     * @param far         Far clip plane distance (> near).
     */
    PerspectiveCamera(float fovRadians, float aspect, float near, float far);

    // ── Camera overrides ──────────────────────────────────────────────────────

    [[nodiscard]] glm::mat4 GetViewMatrix()       const override;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const override;
    [[nodiscard]] glm::vec3 GetPosition()         const override { return m_Position; }
    [[nodiscard]] glm::vec3 GetForward()          const override;
    [[nodiscard]] glm::vec3 GetRight()            const override;
    [[nodiscard]] glm::vec3 GetUp()               const override { return m_Up; }
    [[nodiscard]] float     GetNearPlane()        const override { return m_Near; }
    [[nodiscard]] float     GetFarPlane()         const override { return m_Far;  }

    // ── Setters (mark dirty) ──────────────────────────────────────────────────

    void SetPosition(const glm::vec3& pos);
    void SetTarget  (const glm::vec3& target);
    void SetUp      (const glm::vec3& up);
    void SetFOV     (float radians);
    void SetAspect  (float aspect);
    void SetNearFar (float near_, float far_);

    // ── Extra accessors ───────────────────────────────────────────────────────

    [[nodiscard]] glm::vec3 GetTarget()  const noexcept { return m_Target; }
    [[nodiscard]] float     GetFOV()     const noexcept { return m_FOV;    }
    [[nodiscard]] float     GetAspect()  const noexcept { return m_Aspect; }

private:
    void RecomputeMatrices() const; ///< Updates cached matrices if dirty

    glm::vec3 m_Position  = {0.0f, 0.0f,  5.0f};
    glm::vec3 m_Target    = {0.0f, 0.0f,  0.0f};
    glm::vec3 m_Up        = {0.0f, 1.0f,  0.0f};

    float     m_FOV    = glm::radians(45.0f);
    float     m_Aspect = 16.0f / 9.0f;
    float     m_Near   = 0.01f;
    float     m_Far    = 1000.0f;

    // ── Cached (mutable for lazy compute in const getters) ───────────────────
    mutable glm::mat4 m_View       = glm::mat4(1.0f);
    mutable glm::mat4 m_Proj       = glm::mat4(1.0f);
    mutable bool      m_DirtyView  = true;
    mutable bool      m_DirtyProj  = true;
};

} // namespace Orbital
