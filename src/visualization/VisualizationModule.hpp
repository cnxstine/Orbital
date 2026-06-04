#pragma once

/**
 * @file visualization/VisualizationModule.hpp
 * @brief Lifecycle interface for swappable educational content modules.
 */

#include "core/Error.hpp"
#include <string_view>

namespace Orbital {

/**
 * @brief The VisualizationModule is the abstract interface defining self-contained educational/visualization modules.
 *        It allows instant swapping between modules representing different scientific levels (Orbitals, Bonding, Lattices, Band Structures, etc.) under the controller.
 */
class VisualizationModule {
public:
    virtual ~VisualizationModule() = default;

    /**
     * @brief Triggered when the module becomes active.
     *        Allocates module-specific ECS entities and configures camera systems.
     */
    virtual void OnEnter() = 0;

    /**
     * @brief Triggered when switching away from this module.
     *        Clears all temporary module entities and caches.
     */
    virtual void OnExit() = 0;

    /**
     * @brief Per-frame logic update.
     * @param dt Elapsed delta time in seconds.
     */
    virtual void Update(float dt) = 0;

    /**
     * @brief Enqueues render passes to the scientific rendering system.
     */
    virtual void Render() = 0;

    /**
     * @brief Draw the module-specific ImGui parameters side panel.
     */
    virtual void OnParameterPanel() = 0;
};

} // namespace Orbital
