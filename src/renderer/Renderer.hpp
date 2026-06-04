#pragma once

/**
 * @file renderer/Renderer.hpp
 * @brief High-level render facade — the single point of contact for draw submission.
 *
 * Renderer owns RenderContext and CommandBuffer. It provides:
 *   - BeginFrame(camera): clear buffers, upload camera UBO, reset stats
 *   - Submit(DrawCommand): enqueue a draw call
 *   - EndFrame(): flush command buffer, collect stats
 *
 * All actual OpenGL state changes are confined to this file and renderer/backend/.
 * Higher layers (modules, scene) call only Submit() and query GetStats().
 */

#include "renderer/RenderContext.hpp"
#include "renderer/CommandBuffer.hpp"

#include <glm/glm.hpp>
#include <cstdint>

namespace Orbital {

class Camera;  // Forward — avoid circular header dependency

/// Per-frame rendering statistics.
struct RendererStats {
    uint32_t DrawCalls  = 0;
    uint32_t Triangles  = 0;
    float    FrameTimeMs = 0.0f;
};

// ─────────────────────────────────────────────────────────────────────────────

class Renderer {
public:
    Renderer();
    ~Renderer() = default;

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    void Init(uint32_t viewportWidth, uint32_t viewportHeight);
    void Shutdown();

    // ── Frame interface ────────────────────────────────────────────────────────

    /**
     * @brief Begin a new frame: set viewport, clear buffers, upload camera UBO.
     * @param camera Active camera whose matrices are uploaded to binding 0.
     */
    void BeginFrame(const Camera& camera);

    /**
     * @brief Flush all submitted draw commands and end the frame.
     */
    void EndFrame();

    // ── Draw submission ───────────────────────────────────────────────────────

    /// Enqueue a draw command for execution at EndFrame().
    void Submit(const DrawCommand& cmd);

    // ── Viewport ──────────────────────────────────────────────────────────────

    void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    void SetClearColor(const glm::vec4& color);

    /// Called by Engine on WindowResizeEvent.
    void OnWindowResize(uint32_t width, uint32_t height);

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] RenderContext&       GetContext()      noexcept { return m_Context;    }
    [[nodiscard]] const RenderContext& GetContext() const noexcept { return m_Context;   }
    [[nodiscard]] const RendererStats& GetStats()  const noexcept { return m_Stats;      }

private:
    RenderContext  m_Context;
    CommandBuffer  m_CommandBuffer;

    uint32_t   m_ViewportWidth  = 0;
    uint32_t   m_ViewportHeight = 0;
    glm::vec4  m_ClearColor     = {0.04f, 0.04f, 0.06f, 1.0f}; // Deep dark navy

    RendererStats m_Stats;
    float         m_FrameStart = 0.0f; ///< Time at BeginFrame() for FrameTimeMs
};

} // namespace Orbital
