#pragma once

/**
 * @file scene/components/TransformComponent.hpp
 * @brief Decomposed TRS (Translation, Rotation, Scale) transform component.
 *
 * Stores the transform as independent Position/Rotation/Scale so that
 * individual axes can be manipulated without matrix decomposition artifacts.
 *
 * The local matrix is lazily computed via GetLocalMatrix().
 * The `Dirty` flag lets TransformSystem skip unchanged entities.
 *
 * Quaternion convention: GLM uses w,x,y,z storage; identity = {1,0,0,0}.
 */

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Orbital {

struct TransformComponent {
    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    glm::quat Rotation = glm::identity<glm::quat>(); ///< Identity rotation
    glm::vec3 Scale    = {1.0f, 1.0f, 1.0f};

    bool Dirty = true; ///< True when matrix needs recomputing

    /// Cached world matrix (updated by TransformSystem when Dirty).
    glm::mat4 WorldMatrix = glm::mat4(1.0f);

    /**
     * @brief Compute the local TRS matrix.
     * Does NOT update the cache — TransformSystem does that.
     */
    [[nodiscard]] glm::mat4 GetLocalMatrix() const
    {
        const glm::mat4 T = glm::translate(glm::mat4(1.0f), Position);
        const glm::mat4 R = glm::toMat4(Rotation);
        const glm::mat4 S = glm::scale(glm::mat4(1.0f), Scale);
        return T * R * S;
    }

    // ── Convenience mutators (set Dirty on change) ─────────────────────────────

    void SetPosition(const glm::vec3& p) { Position = p; Dirty = true; }
    void SetRotation(const glm::quat& r) { Rotation = r; Dirty = true; }
    void SetScale   (const glm::vec3& s) { Scale    = s; Dirty = true; }

    void Translate(const glm::vec3& delta) { Position += delta; Dirty = true; }
    void Rotate   (const glm::quat& delta) { Rotation  = delta * Rotation; Dirty = true; }
};

} // namespace Orbital
