#include "renderer/backend/GLVertexArray.hpp"

#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <glad/gl.h>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

static GLenum ToGL(VertexAttributeType type) noexcept
{
    switch (type) {
        case VertexAttributeType::Float: return GL_FLOAT;
        case VertexAttributeType::Int:   return GL_INT;
        case VertexAttributeType::UInt:  return GL_UNSIGNED_INT;
    }
    return GL_FLOAT;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

GLVertexArray::GLVertexArray()
{
    glCreateVertexArrays(1, &m_ArrayID);
    ORB_ASSERT(m_ArrayID != 0, "glCreateVertexArrays failed");
    ORB_CORE_TRACE("GLVertexArray created (id={})", m_ArrayID);
}

GLVertexArray::~GLVertexArray()
{
    if (m_ArrayID) {
        glDeleteVertexArrays(1, &m_ArrayID);
        m_ArrayID = 0;
    }
}

GLVertexArray::GLVertexArray(GLVertexArray&& other) noexcept
    : m_ArrayID(other.m_ArrayID)
    , m_HasIndexBuffer(other.m_HasIndexBuffer)
{
    other.m_ArrayID        = 0;
    other.m_HasIndexBuffer = false;
}

GLVertexArray& GLVertexArray::operator=(GLVertexArray&& other) noexcept
{
    if (this != &other) {
        if (m_ArrayID) glDeleteVertexArrays(1, &m_ArrayID);
        m_ArrayID        = other.m_ArrayID;
        m_HasIndexBuffer = other.m_HasIndexBuffer;
        other.m_ArrayID        = 0;
        other.m_HasIndexBuffer = false;
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
// Buffer association — DSA
// ─────────────────────────────────────────────────────────────────────────────

void GLVertexArray::SetVertexBuffer(const GLBuffer&     vbo,
                                     const VertexLayout& layout,
                                     uint32_t            bindingIndex)
{
    ORB_ASSERT(m_ArrayID != 0,       "GLVertexArray::SetVertexBuffer called on invalid VAO");
    ORB_ASSERT(vbo.IsValid(),        "GLVertexArray::SetVertexBuffer: invalid VBO");
    ORB_ASSERT(layout.stride > 0,    "GLVertexArray::SetVertexBuffer: stride must be > 0");

    // Associate the vertex buffer with the binding index
    glVertexArrayVertexBuffer(m_ArrayID,
                              bindingIndex,
                              vbo.GetID(),
                              /*offset=*/0,
                              static_cast<GLsizei>(layout.stride));

    // Configure each attribute
    for (const auto& attr : layout.attributes) {
        glEnableVertexArrayAttrib(m_ArrayID, attr.index);

        // Set attribute format (float vs integer)
        if (attr.type == VertexAttributeType::Float) {
            glVertexArrayAttribFormat(m_ArrayID,
                                      attr.index,
                                      attr.count,
                                      GL_FLOAT,
                                      attr.normalized ? GL_TRUE : GL_FALSE,
                                      attr.offset);
        } else {
            // Integer formats use glVertexArrayAttribIFormat (no normalisation)
            glVertexArrayAttribIFormat(m_ArrayID,
                                       attr.index,
                                       attr.count,
                                       ToGL(attr.type),
                                       attr.offset);
        }

        // Link the attribute to the binding index
        glVertexArrayAttribBinding(m_ArrayID, attr.index, bindingIndex);
    }
}

void GLVertexArray::SetIndexBuffer(const GLBuffer& ibo)
{
    ORB_ASSERT(m_ArrayID != 0,  "GLVertexArray::SetIndexBuffer called on invalid VAO");
    ORB_ASSERT(ibo.IsValid(),   "GLVertexArray::SetIndexBuffer: invalid IBO");
    ORB_ASSERT(ibo.GetTarget() == BufferTarget::Index,
               "GLVertexArray::SetIndexBuffer: buffer target must be Index");

    glVertexArrayElementBuffer(m_ArrayID, ibo.GetID());
    m_HasIndexBuffer = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Binding
// ─────────────────────────────────────────────────────────────────────────────

void GLVertexArray::Bind() const noexcept
{
    glBindVertexArray(m_ArrayID);
}

void GLVertexArray::Unbind() const noexcept
{
    glBindVertexArray(0);
}

} // namespace Orbital
