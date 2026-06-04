#include "renderer/backend/GLFramebuffer.hpp"

#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <glad/gl.h>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

GLFramebuffer::GLFramebuffer(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
    ORB_ASSERT(width > 0 && height > 0, "GLFramebuffer: dimensions must be > 0");
    glCreateFramebuffers(1, &m_FboID);
    ORB_ASSERT(m_FboID != 0, "glCreateFramebuffers failed");
    ORB_CORE_TRACE("GLFramebuffer created {}x{} (id={})", width, height, m_FboID);
}

GLFramebuffer::~GLFramebuffer()
{
    DestroyFBO();
}

GLFramebuffer::GLFramebuffer(GLFramebuffer&& other) noexcept
    : m_FboID(other.m_FboID)
    , m_Width(other.m_Width)
    , m_Height(other.m_Height)
    , m_Complete(other.m_Complete)
    , m_ColorSpecs(std::move(other.m_ColorSpecs))
    , m_DepthSpec(std::move(other.m_DepthSpec))
    , m_ColorAttachments(std::move(other.m_ColorAttachments))
    , m_DepthAttachment(std::move(other.m_DepthAttachment))
{
    other.m_FboID    = 0;
    other.m_Complete = false;
}

GLFramebuffer& GLFramebuffer::operator=(GLFramebuffer&& other) noexcept
{
    if (this != &other) {
        DestroyFBO();
        m_FboID            = other.m_FboID;
        m_Width            = other.m_Width;
        m_Height           = other.m_Height;
        m_Complete         = other.m_Complete;
        m_ColorSpecs       = std::move(other.m_ColorSpecs);
        m_DepthSpec        = std::move(other.m_DepthSpec);
        m_ColorAttachments = std::move(other.m_ColorAttachments);
        m_DepthAttachment  = std::move(other.m_DepthAttachment);
        other.m_FboID    = 0;
        other.m_Complete = false;
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void GLFramebuffer::DestroyFBO()
{
    if (m_FboID) {
        glDeleteFramebuffers(1, &m_FboID);
        m_FboID    = 0;
        m_Complete = false;
    }
}

void GLFramebuffer::AttachTextures()
{
    // Color attachments
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_ColorAttachments.size()); ++i) {
        glNamedFramebufferTexture(m_FboID,
                                  GL_COLOR_ATTACHMENT0 + i,
                                  m_ColorAttachments[i].GetID(),
                                  0 /*mip level*/);
    }

    // Depth attachment
    if (m_DepthAttachment) {
        glNamedFramebufferTexture(m_FboID,
                                  GL_DEPTH_ATTACHMENT,
                                  m_DepthAttachment->GetID(),
                                  0);
    }

    // Tell GL which color buffers we're drawing into
    if (!m_ColorAttachments.empty()) {
        std::vector<GLenum> drawBuffers;
        drawBuffers.reserve(m_ColorAttachments.size());
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_ColorAttachments.size()); ++i)
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        glNamedFramebufferDrawBuffers(m_FboID,
                                      static_cast<GLsizei>(drawBuffers.size()),
                                      drawBuffers.data());
    } else {
        glNamedFramebufferDrawBuffer(m_FboID, GL_NONE);
        glNamedFramebufferReadBuffer(m_FboID, GL_NONE);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Attachment configuration
// ─────────────────────────────────────────────────────────────────────────────

uint32_t GLFramebuffer::AddColorAttachment(TextureFormat format)
{
    const uint32_t index = static_cast<uint32_t>(m_ColorSpecs.size());
    m_ColorSpecs.push_back({ format });

    TextureSpec spec;
    spec.width     = m_Width;
    spec.height    = m_Height;
    spec.format    = format;
    spec.minFilter = TextureFilter::Linear;
    spec.magFilter = TextureFilter::Linear;
    spec.wrapS     = TextureWrap::ClampToEdge;
    spec.wrapT     = TextureWrap::ClampToEdge;

    m_ColorAttachments.emplace_back(spec);
    return index;
}

void GLFramebuffer::SetDepthAttachment(TextureFormat format)
{
    ORB_ASSERT(format == TextureFormat::Depth32F,
               "GLFramebuffer: only Depth32F is supported for depth attachments");

    m_DepthSpec = { format };

    TextureSpec spec;
    spec.width     = m_Width;
    spec.height    = m_Height;
    spec.format    = format;
    spec.minFilter = TextureFilter::Nearest;
    spec.magFilter = TextureFilter::Nearest;
    spec.wrapS     = TextureWrap::ClampToEdge;
    spec.wrapT     = TextureWrap::ClampToEdge;

    m_DepthAttachment.emplace(spec);
}

// ─────────────────────────────────────────────────────────────────────────────
// Build / validate
// ─────────────────────────────────────────────────────────────────────────────

bool GLFramebuffer::Build()
{
    ORB_ASSERT(m_FboID != 0, "GLFramebuffer::Build called on invalid FBO");

    AttachTextures();

    const GLenum status = glCheckNamedFramebufferStatus(m_FboID, GL_FRAMEBUFFER);
    m_Complete = (status == GL_FRAMEBUFFER_COMPLETE);

    if (!m_Complete) {
        ORB_CORE_ERROR("GLFramebuffer::Build: incomplete FBO (status=0x{:X})", status);
    } else {
        ORB_CORE_TRACE("GLFramebuffer {}x{} complete ({} color, {} depth)",
                       m_Width, m_Height,
                       m_ColorAttachments.size(),
                       m_DepthAttachment.has_value() ? 1 : 0);
    }

    return m_Complete;
}

// ─────────────────────────────────────────────────────────────────────────────
// Binding
// ─────────────────────────────────────────────────────────────────────────────

void GLFramebuffer::Bind() const noexcept
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FboID);
    glViewport(0, 0, static_cast<GLsizei>(m_Width), static_cast<GLsizei>(m_Height));
}

void GLFramebuffer::Unbind() noexcept
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Resize
// ─────────────────────────────────────────────────────────────────────────────

void GLFramebuffer::Resize(uint32_t w, uint32_t h)
{
    ORB_ASSERT(w > 0 && h > 0, "GLFramebuffer::Resize: dimensions must be > 0");

    m_Width    = w;
    m_Height   = h;
    m_Complete = false;

    // Rebuild all color attachments at the new size
    m_ColorAttachments.clear();
    for (const auto& spec : m_ColorSpecs) {
        TextureSpec ts;
        ts.width     = w;
        ts.height    = h;
        ts.format    = spec.format;
        ts.minFilter = TextureFilter::Linear;
        ts.magFilter = TextureFilter::Linear;
        ts.wrapS     = TextureWrap::ClampToEdge;
        ts.wrapT     = TextureWrap::ClampToEdge;
        m_ColorAttachments.emplace_back(ts);
    }

    // Rebuild depth attachment
    if (m_DepthSpec) {
        TextureSpec ts;
        ts.width     = w;
        ts.height    = h;
        ts.format    = m_DepthSpec->format;
        ts.minFilter = TextureFilter::Nearest;
        ts.magFilter = TextureFilter::Nearest;
        ts.wrapS     = TextureWrap::ClampToEdge;
        ts.wrapT     = TextureWrap::ClampToEdge;
        m_DepthAttachment.emplace(ts);
    }

    ORB_CORE_TRACE("GLFramebuffer resized to {}x{}", w, h);
}

// ─────────────────────────────────────────────────────────────────────────────
// Attachment accessors
// ─────────────────────────────────────────────────────────────────────────────

const GLTexture& GLFramebuffer::GetColorAttachment(uint32_t index) const
{
    ORB_ASSERT(index < m_ColorAttachments.size(),
               "GLFramebuffer::GetColorAttachment: index out of range");
    return m_ColorAttachments[index];
}

const GLTexture& GLFramebuffer::GetDepthAttachment() const
{
    ORB_ASSERT(m_DepthAttachment.has_value(),
               "GLFramebuffer::GetDepthAttachment: no depth attachment");
    return *m_DepthAttachment;
}

} // namespace Orbital
