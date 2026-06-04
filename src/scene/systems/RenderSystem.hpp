#pragma once

/**
 * @file scene/systems/RenderSystem.hpp
 * @brief Submits renderable scene entities to the Renderer.
 *
 * Foundation milestone: stub only.
 * Once MeshComponent is added, this system will:
 *   1. Iterate entities with TransformComponent + MeshComponent
 *   2. Bind their material shader
 *   3. Upload per-draw uniforms (u_Model, u_NormalMatrix)
 *   4. Submit DrawCommand to Renderer
 *
 * For now, it intentionally does nothing — the renderer clears to black,
 * which is sufficient for the "launch a black window" goal.
 */

namespace Orbital {
    class Scene;
    class Renderer;
    class RenderContext;
}

namespace Orbital {

class RenderSystem {
public:
    RenderSystem() = delete;

    /**
     * @brief Submit all renderable entities to the renderer.
     * @param scene    Scene to query for renderable entities.
     * @param renderer Target renderer (Submit() enqueues draw calls).
     * @param ctx      Current frame's render context (camera UBO, viewport).
     */
    static void OnUpdate(Scene& scene, Renderer& renderer, RenderContext& ctx);
};

} // namespace Orbital
