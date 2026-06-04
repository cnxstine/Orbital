#pragma once

/**
 * @file scene/systems/TransformSystem.hpp
 * @brief Recomputes world matrices for dirty TransformComponents.
 *
 * Called once per frame by SceneLayer before RenderSystem, ensuring all
 * world matrices are up-to-date before any draw submission.
 *
 * For the foundation milestone (no hierarchy / parenting):
 *   WorldMatrix = GetLocalMatrix()
 *
 * Future: parent-child hierarchy traversal (topological sort + propagate).
 */

namespace Orbital { class Scene; }

namespace Orbital {

class TransformSystem {
public:
    TransformSystem() = delete; ///< Static utility class — no instances

    /// Iterate all TransformComponents and recompute WorldMatrix where Dirty.
    static void OnUpdate(Scene& scene);
};

} // namespace Orbital
