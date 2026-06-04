#pragma once

/**
 * @file renderer/backend/GLBuffer.hpp
 * @brief RAII wrapper around an OpenGL buffer object (VBO, IBO, UBO, SSBO, etc.).
 *
 * Uses OpenGL 4.5 Direct State Access (DSA):
 *   glCreateBuffers  (not glGenBuffers)
 *   glNamedBufferData / glNamedBufferSubData
 *
 * Move-only: copy is disabled to enforce single ownership of the GL object.
 *
 * Usage:
 *   GLBuffer vbo(BufferTarget::Vertex, BufferUsage::StaticDraw);
 *   vbo.Upload(vertices.data(), vertices.size() * sizeof(Vertex));
 *   vbo.Bind();
 */

#include "core/Assert.hpp"

#include <cstdint>
#include <cstddef>
#include <span>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────

/// Specifies the intended target binding point for the buffer.
enum class BufferTarget : uint32_t {
    Vertex,          ///< GL_ARRAY_BUFFER
    Index,           ///< GL_ELEMENT_ARRAY_BUFFER
    Uniform,         ///< GL_UNIFORM_BUFFER
    ShaderStorage,   ///< GL_SHADER_STORAGE_BUFFER
    DrawIndirect,    ///< GL_DRAW_INDIRECT_BUFFER
};

/// Hints the GL driver about access frequency and nature.
enum class BufferUsage : uint32_t {
    StaticDraw,   ///< GL_STATIC_DRAW  — set once, read many
    DynamicDraw,  ///< GL_DYNAMIC_DRAW — updated often, read many
    StreamDraw,   ///< GL_STREAM_DRAW  — set once, read once
    StaticRead,   ///< GL_STATIC_READ  — set by GL, read by app
};

// ─────────────────────────────────────────────────────────────────────────────

class GLBuffer {
public:
    // ── Construction / destruction ─────────────────────────────────────────────

    /**
     * @brief Allocate a new GL buffer object (DSA: glCreateBuffers).
     * @param target  Intended binding target.
     * @param usage   Access pattern hint.
     */
    GLBuffer(BufferTarget target, BufferUsage usage);
    ~GLBuffer();

    // Move-only
    GLBuffer(GLBuffer&& other) noexcept;
    GLBuffer& operator=(GLBuffer&& other) noexcept;

    GLBuffer(const GLBuffer&)            = delete;
    GLBuffer& operator=(const GLBuffer&) = delete;

    // ── Data upload ───────────────────────────────────────────────────────────

    /**
     * @brief Allocate and upload data (replaces any prior allocation).
     * @param data       Pointer to the source data; may be nullptr to just allocate.
     * @param sizeBytes  Size of the allocation in bytes.
     */
    void Upload(const void* data, size_t sizeBytes);

    /**
     * @brief Update a sub-range of an already-allocated buffer.
     * @param data       Source data.
     * @param offset     Byte offset into the existing buffer.
     * @param sizeBytes  Number of bytes to update.
     */
    void UploadSubData(const void* data, size_t offset, size_t sizeBytes);

    /**
     * @brief Typed convenience wrapper — uploads a contiguous span.
     */
    template<typename T>
    void Upload(std::span<const T> data) {
        Upload(data.data(), data.size_bytes());
    }

    // ── Binding ───────────────────────────────────────────────────────────────

    void Bind()   const noexcept;
    void Unbind() const noexcept;

    /**
     * @brief Bind the buffer to an indexed binding point (UBO / SSBO).
     * @param bindingPoint  The layout(binding=N) index in GLSL.
     */
    void BindBase(uint32_t bindingPoint) const noexcept;

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] uint32_t GetID()        const noexcept { return m_BufferID;   }
    [[nodiscard]] size_t   GetSizeBytes() const noexcept { return m_SizeBytes;  }
    [[nodiscard]] BufferTarget GetTarget()const noexcept { return m_Target;     }
    [[nodiscard]] bool     IsValid()      const noexcept { return m_BufferID != 0; }

private:
    uint32_t     m_BufferID  = 0;
    size_t       m_SizeBytes = 0;
    BufferTarget m_Target;
    BufferUsage  m_Usage;
};

} // namespace Orbital
