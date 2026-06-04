#include "renderer/CommandBuffer.hpp"
#include "renderer/RenderContext.hpp"

#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <glad/gl.h>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────

void CommandBuffer::Clear() noexcept
{
    m_Commands.clear();
}

void CommandBuffer::Submit(const DrawCommand& cmd)
{
    m_Commands.push_back(cmd);
}

// ─────────────────────────────────────────────────────────────────────────────

void CommandBuffer::Execute(RenderContext& ctx)
{
    for (const DrawCommand& cmd : m_Commands) {
        // Bind shader program
        if (cmd.shaderProgramId != 0)
            glUseProgram(cmd.shaderProgramId);

        // Bind VAO
        glBindVertexArray(cmd.vaoId);

        // Issue the draw call
        if (cmd.indexCount > 0) {
            // Indexed draw
            if (cmd.instanceCount > 1) {
                glDrawElementsInstanced(
                    static_cast<GLenum>(cmd.primitiveType),
                    static_cast<GLsizei>(cmd.indexCount),
                    static_cast<GLenum>(cmd.indexType),
                    /*indices=*/nullptr,
                    static_cast<GLsizei>(cmd.instanceCount));
            } else {
                glDrawElements(
                    static_cast<GLenum>(cmd.primitiveType),
                    static_cast<GLsizei>(cmd.indexCount),
                    static_cast<GLenum>(cmd.indexType),
                    /*indices=*/nullptr);
            }
        } else {
            // Non-indexed draw
            if (cmd.instanceCount > 1) {
                glDrawArraysInstanced(
                    static_cast<GLenum>(cmd.primitiveType),
                    0,
                    static_cast<GLsizei>(cmd.vertexCount),
                    static_cast<GLsizei>(cmd.instanceCount));
            } else {
                glDrawArrays(
                    static_cast<GLenum>(cmd.primitiveType),
                    0,
                    static_cast<GLsizei>(cmd.vertexCount));
            }
        }

        ctx.IncrementDrawCalls();
    }

    // Cleanup state
    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace Orbital
