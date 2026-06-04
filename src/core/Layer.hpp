#pragma once

/**
 * @file core/Layer.hpp
 * @brief Abstract base for all layers in the LayerStack.
 *
 * A Layer represents a discrete logical subsystem that participates in:
 *   - Per-frame update
 *   - Render submission
 *   - Event handling (input events propagate top-to-bottom)
 *   - ImGui rendering
 *
 * Concrete layers (as planned by the architecture):
 *   DebugLayer, UILayer, ModuleLayer, SceneLayer, SimulationLayer
 *
 * Ownership: LayerStack owns all Layer* via std::unique_ptr.
 */

#include "events/Event.hpp"

#include <string>
#include <string_view>

namespace Orbital {

class Layer {
public:
    explicit Layer(std::string_view name = "Layer") : m_Name(name) {}
    virtual ~Layer() = default;

    Layer(const Layer&)            = delete;
    Layer& operator=(const Layer&) = delete;
    Layer(Layer&&)                 = delete;
    Layer& operator=(Layer&&)      = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /// Called once when the layer is pushed onto the stack.
    virtual void OnAttach() {}

    /// Called once when the layer is popped from the stack.
    virtual void OnDetach() {}

    // ── Per-frame callbacks ────────────────────────────────────────────────────

    /// Variable timestep update (animations, input processing).
    virtual void OnUpdate(float dt) {}

    /// Fixed timestep update (physics proxies, predictable logic).
    virtual void OnFixedUpdate(float fixedDt) {}

    /// Submit draw calls to the Renderer. Called after all OnUpdate().
    virtual void OnRender() {}

    /// Emit Dear ImGui calls. Called after all OnRender().
    virtual void OnImGui() {}

    // ── Event handling ────────────────────────────────────────────────────────

    /**
     * @brief Handle an incoming event.
     * @param event The event to handle.
     * @return true if the event is consumed (stops propagation to lower layers).
     */
    virtual bool OnEvent(Event& event) { return false; }

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] std::string_view GetName()    const noexcept { return m_Name; }
    [[nodiscard]] bool             IsEnabled()  const noexcept { return m_Enabled; }
    void                           SetEnabled(bool v) noexcept { m_Enabled = v; }

private:
    std::string m_Name;
    bool        m_Enabled = true;
};

} // namespace Orbital
