#include "camera/CameraManager.hpp"
#include "events/events/WindowEvents.hpp"
#include "events/events/InputEvents.hpp"
#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <imgui.h>

namespace Orbital {

CameraManager::CameraManager(EventBus& bus)
{
    // Window resize → update camera aspect ratio
    m_ResizeToken = bus.Subscribe<WindowResizeEvent>(
        [this](const WindowResizeEvent& e) {
            OnWindowResize(e);
            return false;
        });

    // Mouse input → forward to active controller
    m_MouseButtonToken = bus.Subscribe<MouseButtonPressedEvent>(
        [this](const MouseButtonPressedEvent& e) {
            if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse) {
                const_cast<MouseButtonPressedEvent&>(e).Handled = true;
                return true;
            }
            if (!m_Controller) return false;
            return m_Controller->OnEvent(const_cast<MouseButtonPressedEvent&>(e));
        });

    m_MouseButtonRelToken = bus.Subscribe<MouseButtonReleasedEvent>(
        [this](const MouseButtonReleasedEvent& e) {
            if (!m_Controller) return false;
            return m_Controller->OnEvent(const_cast<MouseButtonReleasedEvent&>(e));
        });

    m_MouseMovedToken = bus.Subscribe<MouseMovedEvent>(
        [this](const MouseMovedEvent& e) {
            if (!m_Controller) return false;
            return m_Controller->OnEvent(const_cast<MouseMovedEvent&>(e));
        });

    m_MouseScrollToken = bus.Subscribe<MouseScrolledEvent>(
        [this](const MouseScrolledEvent& e) {
            if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse) {
                const_cast<MouseScrolledEvent&>(e).Handled = true;
                return true;
            }
            if (!m_Controller) return false;
            return m_Controller->OnEvent(const_cast<MouseScrolledEvent&>(e));
        });
}

// ── Camera assignment ─────────────────────────────────────────────────────────

void CameraManager::SetCamera(std::unique_ptr<PerspectiveCamera> camera)
{
    m_Camera = std::move(camera);

    // Notify the controller about the new camera
    if (m_Controller && m_Camera) {
        m_Controller->SetCamera(m_Camera.get());
    }

    ORB_CORE_TRACE("CameraManager: camera set");
}

void CameraManager::SetController(std::unique_ptr<CameraController> controller)
{
    m_Controller = std::move(controller);

    // Bind immediately if a camera already exists
    if (m_Controller && m_Camera) {
        m_Controller->SetCamera(m_Camera.get());
    }

    ORB_CORE_TRACE("CameraManager: controller set");
}

// ── Per-frame ─────────────────────────────────────────────────────────────────

void CameraManager::OnUpdate(float dt)
{
    if (m_Controller) m_Controller->OnUpdate(dt);
}

bool CameraManager::OnEvent(Event& event)
{
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse) {
        if (event.GetType() == EventType::MouseButtonPressed || event.GetType() == EventType::MouseScrolled) {
            event.Handled = true;
            return true;
        }
    }
    if (m_Controller) return m_Controller->OnEvent(event);
    return false;
}

// ── Window resize ─────────────────────────────────────────────────────────────

void CameraManager::OnWindowResize(const WindowResizeEvent& e)
{
    if (!m_Camera) return;
    if (e.Width == 0 || e.Height == 0) return; // Minimised

    const float aspect = static_cast<float>(e.Width) / static_cast<float>(e.Height);
    m_Camera->SetAspect(aspect);
}

} // namespace Orbital
