#pragma once

/**
 * @file camera/CameraController.hpp
 * @brief Abstract base for all camera input controllers.
 *
 * Separation principle: Camera holds data (matrices, position, target).
 * CameraController interprets input and mutates the Camera.
 *
 * Concrete controllers:
 *   ArcballController  — rotate/pan/dolly around a pivot (default for orbital viewing)
 *   FlyController      — free-fly 6DOF navigation
 *   CinematicController — keyframe spline animation (future)
 *
 * Controllers receive raw events from CameraManager::OnEvent() and
 * per-frame dt via CameraManager::OnUpdate().
 */

#include "events/Event.hpp"

namespace Orbital {

class PerspectiveCamera; // Forward — controllers work on PerspectiveCamera

class CameraController {
public:
    virtual ~CameraController() = default;

    CameraController(const CameraController&)            = delete;
    CameraController& operator=(const CameraController&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /// Bind to a camera. Called by CameraManager when controller is set.
    virtual void SetCamera(PerspectiveCamera* camera) { m_Camera = camera; }

    // ── Per-frame ─────────────────────────────────────────────────────────────

    /// Variable-timestep update: read input, move camera.
    virtual void OnUpdate(float dt) = 0;

    // ── Event handling ────────────────────────────────────────────────────────

    /**
     * @brief Handle a raw input event (mouse buttons, scroll, keyboard).
     * @return true if the event was consumed (stops LayerStack propagation).
     */
    virtual bool OnEvent(Event& event) = 0;

protected:
    CameraController() = default;

    PerspectiveCamera* m_Camera = nullptr; ///< Non-owning; owned by CameraManager
};

} // namespace Orbital
