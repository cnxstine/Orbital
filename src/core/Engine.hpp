#pragma once

/**
 * @file core/Engine.hpp
 * @brief The central orchestrator: owns all subsystems and drives the main loop.
 *
 * Engine is the single source of truth for subsystem lifetime. It:
 *  1. Creates and owns all subsystems (Window, EventBus, ResourceManager, etc.)
 *  2. Provides typed access via Engine::Get<T>() (ServiceLocator pattern).
 *  3. Drives the main loop: PollEvents → Update → Render → Swap.
 *  4. Manages the LayerStack (layers register themselves via PushLayer).
 *
 * Usage:
 *   Engine engine(spec);
 *   engine.PushLayer(std::make_unique<MyLayer>());
 *   engine.Run(); // blocks until window close
 *
 * The Engine does NOT use global singletons. All state is instance-owned.
 * Application owns the Engine.
 */

#include "core/LayerStack.hpp"
#include "core/Time.hpp"
#include "events/EventBus.hpp"
#include "platform/Window.hpp"
#include "resources/ResourceManager.hpp"

#include <memory>
#include <functional>

// Forward declarations — avoids circular includes
namespace Orbital {
    class Renderer;
    class CameraManager;
}

namespace Orbital {

struct EngineSpec {
    WindowSpec Window;
};

class Engine {
public:
    explicit Engine(const EngineSpec& spec);
    ~Engine();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    // ── Main loop ─────────────────────────────────────────────────────────────

    /// Starts the blocking main loop. Returns when the window is closed.
    void Run();

    // ── Layer management ──────────────────────────────────────────────────────

    /// Push a layer below overlays. Returns raw (non-owning) pointer.
    Layer* PushLayer(std::unique_ptr<Layer> layer);

    /// Push an overlay on top of all layers. Returns raw (non-owning) pointer.
    Layer* PushOverlay(std::unique_ptr<Layer> overlay);

    // ── Subsystem access ──────────────────────────────────────────────────────

    [[nodiscard]] EventBus&         GetEventBus()      noexcept { return m_EventBus; }
    [[nodiscard]] Window&           GetWindow()        noexcept { return *m_Window; }
    [[nodiscard]] ResourceManager&  GetResourceManager() noexcept { return m_ResourceManager; }
    [[nodiscard]] Renderer&         GetRenderer()      noexcept { return *m_Renderer; }
    [[nodiscard]] CameraManager&    GetCameraManager() noexcept { return *m_CameraManager; }
    [[nodiscard]] LayerStack&       GetLayerStack()    noexcept { return m_LayerStack; }

    // ── Lifecycle signal ──────────────────────────────────────────────────────

    void RequestShutdown() noexcept { m_Running = false; }

private:
    void TickFrame(float dt);

    // ── Subsystems ────────────────────────────────────────────────────────────
    EventBus        m_EventBus;                  // Layer 1 — owns event dispatch
    ResourceManager m_ResourceManager;           // Layer 1 — owns resource cache

    std::unique_ptr<Window>         m_Window;         // Layer 0 — GLFW wrapper
    std::unique_ptr<Renderer>       m_Renderer;       // Layer 2 — render facade
    std::unique_ptr<CameraManager>  m_CameraManager;  // Layer 2 — active camera

    LayerStack m_LayerStack;

    bool     m_Running         = false;
    float    m_FixedAccumulator = 0.0f;

    SubscriptionToken m_CloseToken;
    SubscriptionToken m_ResizeToken;
};

} // namespace Orbital
