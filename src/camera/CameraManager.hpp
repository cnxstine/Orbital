#pragma once

/**
 * @file camera/CameraManager.hpp
 * @brief Manages the active camera and its controller.
 *
 * CameraManager is the bridge between Engine, the active Camera, and the
 * active CameraController. It owns both via unique_ptr and wires them together.
 *
 * It subscribes to WindowResizeEvent to keep the camera aspect ratio current.
 * Event routing: Engine calls CameraManager::OnEvent() for each dispatched event;
 * CameraManager forwards to the active controller.
 */

#include "camera/PerspectiveCamera.hpp"
#include "camera/CameraController.hpp"
#include "events/EventBus.hpp"

#include <memory>

namespace Orbital {

class CameraManager {
public:
    explicit CameraManager(EventBus& bus);
    ~CameraManager() = default;

    CameraManager(const CameraManager&)            = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    // ── Camera / Controller assignment ────────────────────────────────────────

    /**
     * @brief Replace the active camera. Notifies the controller of the change.
     */
    void SetCamera(std::unique_ptr<PerspectiveCamera> camera);

    /**
     * @brief Replace the active controller. Binds it to the current camera.
     */
    void SetController(std::unique_ptr<CameraController> controller);

    // ── Per-frame ─────────────────────────────────────────────────────────────

    /// Forward variable-dt update to the active controller.
    void OnUpdate(float dt);

    /// Route an event to the active controller.
    bool OnEvent(Event& event);

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] PerspectiveCamera*   GetCamera()     noexcept { return m_Camera.get();     }
    [[nodiscard]] CameraController*    GetController() noexcept { return m_Controller.get(); }

    [[nodiscard]] const PerspectiveCamera* GetCamera() const noexcept { return m_Camera.get(); }

private:
    void OnWindowResize(const struct WindowResizeEvent& e);

    std::unique_ptr<PerspectiveCamera> m_Camera;
    std::unique_ptr<CameraController>  m_Controller;

    SubscriptionToken m_ResizeToken;
    SubscriptionToken m_MouseButtonToken;
    SubscriptionToken m_MouseButtonRelToken;
    SubscriptionToken m_MouseMovedToken;
    SubscriptionToken m_MouseScrollToken;
};

} // namespace Orbital
