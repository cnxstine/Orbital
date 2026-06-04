#pragma once

/**
 * @file core/LayerStack.hpp
 * @brief Ordered stack of Layer objects managed by the Engine.
 *
 * Layers are partitioned into two regions:
 *
 *   [0 .. m_LayerInsert)   → regular layers  (pushed at the "bottom")
 *   [m_LayerInsert .. end) → overlay layers  (pushed at the "top")
 *
 * Regular layers execute first in update order; overlays execute last.
 * For event propagation the order is REVERSED: overlays handle events
 * before regular layers (top-most overlay gets first crack at input).
 *
 * Example state with 2 layers + 1 overlay:
 *   [SceneLayer, UILayer | DebugOverlay]
 *                         ^-- m_LayerInsert
 *
 * Update order  : SceneLayer → UILayer → DebugOverlay
 * Event order   : DebugOverlay → UILayer → SceneLayer
 */

#include "core/Layer.hpp"

#include <vector>
#include <memory>
#include <string_view>

namespace Orbital {

class LayerStack {
public:
    LayerStack()  = default;
    ~LayerStack() = default;

    LayerStack(const LayerStack&)            = delete;
    LayerStack& operator=(const LayerStack&) = delete;

    // ── Mutation ──────────────────────────────────────────────────────────────

    /**
     * @brief Push a layer below all overlays.
     * @param layer Ownership is transferred to this stack.
     * @return Raw pointer for caller convenience (non-owning).
     */
    Layer* PushLayer(std::unique_ptr<Layer> layer);

    /**
     * @brief Push an overlay on top of all other layers.
     * @param overlay Ownership is transferred to this stack.
     * @return Raw pointer for caller convenience (non-owning).
     */
    Layer* PushOverlay(std::unique_ptr<Layer> overlay);

    /// Remove a layer by name and destroy it.
    void PopLayer(std::string_view name);

    /// Remove an overlay by name and destroy it.
    void PopOverlay(std::string_view name);

    /// Detach and destroy all layers.
    void Clear();

    // ── Iteration ─────────────────────────────────────────────────────────────

    using Container = std::vector<std::unique_ptr<Layer>>;

    /// Forward iteration: update/render order (bottom → top).
    [[nodiscard]] Container::iterator begin() noexcept { return m_Layers.begin(); }
    [[nodiscard]] Container::iterator end()   noexcept { return m_Layers.end();   }

    /// Reverse iteration: event propagation order (top → bottom).
    [[nodiscard]] Container::reverse_iterator rbegin() noexcept { return m_Layers.rbegin(); }
    [[nodiscard]] Container::reverse_iterator rend()   noexcept { return m_Layers.rend();   }

    [[nodiscard]] bool  Empty() const noexcept { return m_Layers.empty(); }
    [[nodiscard]] std::size_t Size()  const noexcept { return m_Layers.size(); }

private:
    Container   m_Layers;
    std::size_t m_LayerInsert = 0; ///< Insertion point separating layers from overlays
};

} // namespace Orbital
