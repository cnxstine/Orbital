#pragma once

/**
 * @file scene/components/CameraComponent.hpp
 * @brief Tag component that marks an entity as the active scene camera.
 *
 * An entity with CameraComponent + TransformComponent can drive a Camera.
 * CameraManager queries the scene for the entity with Active == true and
 * syncs the Camera position from its TransformComponent each frame.
 *
 * Only one entity should have Active == true at a time.
 */

namespace Orbital {

struct CameraComponent {
    bool Active = false; ///< Set to true for the primary camera entity
};

} // namespace Orbital
