#pragma once

/**
 * @file renderer/CommandBuffer.hpp
 * @brief Records draw commands for deferred submission to the GPU.
 *
 * DrawCommands are enqueued via Submit() and flushed all at once via
 * Execute(). This two-phase approach enables:
 *   - Sorting by shader / VAO before submission (future optimization)
 *   - Per-frame statistics gathering
 *   - Easy replay for debugging
 *
 * Execute() makes direct OpenGL draw calls — it must be called from
 * the main (GL) thread.
 *
 * Usage:
 *   cmd.Clear();
 *   cmd.Submit({ vao.GetID(), shader.GetProgramID(), 0, 36 });
 *   cmd.Execute(ctx);
 */

#include <cstdint>
#include <vector>

namespace Orbital {

class RenderContext; // Forward declaration

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single recorded draw call.
 */
struct DrawCommand {
    uint32_t vaoId          = 0;            ///< GLVertexArray object ID
    uint32_t shaderProgramId = 0;           ///< GLShader program ID
    uint32_t vertexCount    = 0;            ///< # vertices (used for DrawArrays)
    uint32_t indexCount     = 0;            ///< # indices  (used for DrawElements; 0 = DrawArrays)
    uint32_t instanceCount  = 1;            ///< Number of instances
    uint32_t primitiveType  = 0x0004;       ///< GL_TRIANGLES (0x0004) by default
    uint32_t indexType      = 0x1405;       ///< GL_UNSIGNED_INT (0x1405) by default
};

// ─────────────────────────────────────────────────────────────────────────────

class CommandBuffer {
public:
    CommandBuffer()  = default;
    ~CommandBuffer() = default;

    // ── Recording ─────────────────────────────────────────────────────────────

    /// Reset the command list for a new frame.
    void Clear() noexcept;

    /**
     * @brief Enqueue a draw command for execution.
     * @param cmd  The draw call parameters.
     */
    void Submit(const DrawCommand& cmd);

    // ── Execution ─────────────────────────────────────────────────────────────

    /**
     * @brief Flush all recorded commands to the GPU.
     *        Increments RenderContext draw-call counter for each command.
     * @param ctx  Active RenderContext (used for stats).
     */
    void Execute(RenderContext& ctx);

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] size_t GetCommandCount() const noexcept { return m_Commands.size(); }

private:
    std::vector<DrawCommand> m_Commands;
};

} // namespace Orbital
