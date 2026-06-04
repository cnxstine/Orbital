#include "renderer/backend/GLBuffer.hpp"

#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <glad/gl.h>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers: enum → GL constant
// ─────────────────────────────────────────────────────────────────────────────

static GLenum ToGL(BufferTarget target) noexcept
{
    switch (target) {
        case BufferTarget::Vertex:        return GL_ARRAY_BUFFER;
        case BufferTarget::Index:         return GL_ELEMENT_ARRAY_BUFFER;
        case BufferTarget::Uniform:       return GL_UNIFORM_BUFFER;
        case BufferTarget::ShaderStorage: return GL_SHADER_STORAGE_BUFFER;
        case BufferTarget::DrawIndirect:  return GL_DRAW_INDIRECT_BUFFER;
    }
    return GL_ARRAY_BUFFER;
}

static GLenum ToGL(BufferUsage usage) noexcept
{
    switch (usage) {
        case BufferUsage::StaticDraw:  return GL_STATIC_DRAW;
        case BufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
        case BufferUsage::StreamDraw:  return GL_STREAM_DRAW;
        case BufferUsage::StaticRead:  return GL_STATIC_READ;
    }
    return GL_STATIC_DRAW;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

GLBuffer::GLBuffer(BufferTarget target, BufferUsage usage)
    : m_Target(target), m_Usage(usage)
{
    glCreateBuffers(1, &m_BufferID);
    ORB_ASSERT(m_BufferID != 0, "glCreateBuffers failed");
    ORB_CORE_TRACE("GLBuffer created (id={})", m_BufferID);
}

GLBuffer::~GLBuffer()
{
    if (m_BufferID) {
        glDeleteBuffers(1, &m_BufferID);
        m_BufferID = 0;
    }
}

GLBuffer::GLBuffer(GLBuffer&& other) noexcept
    : m_BufferID(other.m_BufferID)
    , m_SizeBytes(other.m_SizeBytes)
    , m_Target(other.m_Target)
    , m_Usage(other.m_Usage)
{
    other.m_BufferID  = 0;
    other.m_SizeBytes = 0;
}

GLBuffer& GLBuffer::operator=(GLBuffer&& other) noexcept
{
    if (this != &other) {
        if (m_BufferID) glDeleteBuffers(1, &m_BufferID);
        m_BufferID  = other.m_BufferID;
        m_SizeBytes = other.m_SizeBytes;
        m_Target    = other.m_Target;
        m_Usage     = other.m_Usage;
        other.m_BufferID  = 0;
        other.m_SizeBytes = 0;
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
// Data upload
// ─────────────────────────────────────────────────────────────────────────────

void GLBuffer::Upload(const void* data, size_t sizeBytes)
{
    ORB_ASSERT(m_BufferID != 0, "GLBuffer::Upload called on invalid buffer");
    glNamedBufferData(m_BufferID,
                      static_cast<GLsizeiptr>(sizeBytes),
                      data,
                      ToGL(m_Usage));
    m_SizeBytes = sizeBytes;
}

void GLBuffer::UploadSubData(const void* data, size_t offset, size_t sizeBytes)
{
    ORB_ASSERT(m_BufferID != 0,    "GLBuffer::UploadSubData called on invalid buffer");
    ORB_ASSERT(offset + sizeBytes <= m_SizeBytes,
               "GLBuffer::UploadSubData: range exceeds buffer allocation");

    glNamedBufferSubData(m_BufferID,
                         static_cast<GLintptr>(offset),
                         static_cast<GLsizeiptr>(sizeBytes),
                         data);
}

// ─────────────────────────────────────────────────────────────────────────────
// Binding
// ─────────────────────────────────────────────────────────────────────────────

void GLBuffer::Bind() const noexcept
{
    glBindBuffer(ToGL(m_Target), m_BufferID);
}

void GLBuffer::Unbind() const noexcept
{
    glBindBuffer(ToGL(m_Target), 0);
}

void GLBuffer::BindBase(uint32_t bindingPoint) const noexcept
{
    ORB_ASSERT(m_Target == BufferTarget::Uniform ||
               m_Target == BufferTarget::ShaderStorage,
               "BindBase is only valid for UBO and SSBO targets");
    glBindBufferBase(ToGL(m_Target), bindingPoint, m_BufferID);
}

} // namespace Orbital
