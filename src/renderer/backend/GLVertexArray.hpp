#pragma once

/**
 * @file renderer/backend/GLVertexArray.hpp
 * @brief RAII wrapper around an OpenGL Vertex Array Object (VAO).
 *
 * Describes the vertex layout (attribute format, stride, offsets) and
 * associates vertex + index buffers using OpenGL 4.5 DSA calls:
 *   glCreateVertexArrays
 *   glVertexArrayVertexBuffer
 *   glVertexArrayAttribFormat / glVertexArrayAttribIFormat
 *   glVertexArrayAttribBinding
 *   glEnableVertexArrayAttrib
 *   glVertexArrayElementBuffer
 *
 * Move-only; the destructor calls glDeleteVertexArrays.
 *
 * Usage:
 *   VertexLayout layout;
 *   layout.stride = sizeof(Vertex);
 *   layout.attributes = {
 *       {0, 3, VertexAttributeType::Float, false, offsetof(Vertex, position)},
 *       {1, 3, VertexAttributeType::Float, false, offsetof(Vertex, normal)},
 *       {2, 2, VertexAttributeType::Float, false, offsetof(Vertex, texCoord)},
 *   };
 *   GLVertexArray vao;
 *   vao.SetVertexBuffer(vbo, layout);
 *   vao.SetIndexBuffer(ibo);
 */

#include "renderer/backend/GLBuffer.hpp"

#include <cstdint>
#include <vector>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────

/// Underlying GLSL attribute data type.
enum class VertexAttributeType : uint32_t {
    Float,
    Int,
    UInt,
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Describes a single vertex attribute binding.
 */
struct VertexAttribute {
    uint32_t             index;       ///< layout(location = N) in GLSL
    int                  count;       ///< Number of components (1–4)
    VertexAttributeType  type;        ///< Data type per component
    bool                 normalized;  ///< Normalize integer types to [0,1]
    uint32_t             offset;      ///< Byte offset within the vertex struct
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Describes the complete layout of a vertex buffer.
 */
struct VertexLayout {
    std::vector<VertexAttribute> attributes;
    uint32_t                     stride = 0; ///< Total bytes per vertex
};

// ─────────────────────────────────────────────────────────────────────────────

class GLVertexArray {
public:
    // ── Construction / destruction ─────────────────────────────────────────────

    GLVertexArray();
    ~GLVertexArray();

    // Move-only
    GLVertexArray(GLVertexArray&& other) noexcept;
    GLVertexArray& operator=(GLVertexArray&& other) noexcept;

    GLVertexArray(const GLVertexArray&)            = delete;
    GLVertexArray& operator=(const GLVertexArray&) = delete;

    // ── Buffer association ────────────────────────────────────────────────────

    /**
     * @brief Associate a vertex buffer and specify its attribute layout.
     * @param vbo           The vertex buffer to bind.
     * @param layout        Attribute format and stride description.
     * @param bindingIndex  Vertex buffer binding slot (0 for most use cases).
     */
    void SetVertexBuffer(const GLBuffer&     vbo,
                         const VertexLayout& layout,
                         uint32_t            bindingIndex = 0);

    /**
     * @brief Associate an index (element) buffer with the VAO.
     * @param ibo  The index buffer to attach.
     */
    void SetIndexBuffer(const GLBuffer& ibo);

    // ── Binding ───────────────────────────────────────────────────────────────

    void Bind()   const noexcept;
    void Unbind() const noexcept;

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] uint32_t GetID()      const noexcept { return m_ArrayID;       }
    [[nodiscard]] bool     HasIndices() const noexcept { return m_HasIndexBuffer; }
    [[nodiscard]] bool     IsValid()    const noexcept { return m_ArrayID != 0;   }

private:
    uint32_t m_ArrayID        = 0;
    bool     m_HasIndexBuffer = false;
};

} // namespace Orbital
