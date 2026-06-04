#include "renderer/backend/GLTexture.hpp"

#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <glad/gl.h>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Internal format mapping helpers
// ─────────────────────────────────────────────────────────────────────────────

struct GLFormatInfo {
    GLenum internalFormat;  ///< Sized internal format (e.g. GL_RGBA8)
    GLenum baseFormat;      ///< Base format for upload (e.g. GL_RGBA)
    GLenum dataType;        ///< Data type for upload (e.g. GL_UNSIGNED_BYTE)
};

static GLFormatInfo GetFormatInfo(TextureFormat fmt) noexcept
{
    switch (fmt) {
        case TextureFormat::R8:      return { GL_R8,                GL_RED,             GL_UNSIGNED_BYTE  };
        case TextureFormat::RG8:     return { GL_RG8,               GL_RG,              GL_UNSIGNED_BYTE  };
        case TextureFormat::RGB8:    return { GL_RGB8,              GL_RGB,             GL_UNSIGNED_BYTE  };
        case TextureFormat::RGBA8:   return { GL_RGBA8,             GL_RGBA,            GL_UNSIGNED_BYTE  };
        case TextureFormat::R32F:    return { GL_R32F,              GL_RED,             GL_FLOAT          };
        case TextureFormat::RGB16F:  return { GL_RGB16F,            GL_RGB,             GL_FLOAT          };
        case TextureFormat::RGBA16F: return { GL_RGBA16F,           GL_RGBA,            GL_FLOAT          };
        case TextureFormat::RGBA32F: return { GL_RGBA32F,           GL_RGBA,            GL_FLOAT          };
        case TextureFormat::Depth32F:return { GL_DEPTH_COMPONENT32F,GL_DEPTH_COMPONENT, GL_FLOAT          };
    }
    return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
}

static GLenum ToGL(TextureFilter filter) noexcept
{
    switch (filter) {
        case TextureFilter::Nearest:            return GL_NEAREST;
        case TextureFilter::Linear:             return GL_LINEAR;
        case TextureFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
}

static GLenum ToGL(TextureWrap wrap) noexcept
{
    switch (wrap) {
        case TextureWrap::Repeat:        return GL_REPEAT;
        case TextureWrap::ClampToEdge:   return GL_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder: return GL_CLAMP_TO_BORDER;
    }
    return GL_CLAMP_TO_EDGE;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private: allocate immutable storage
// ─────────────────────────────────────────────────────────────────────────────

void GLTexture::AllocateStorage()
{
    const bool is3D = (m_Spec.depth > 1);

    // Determine mip levels
    int mipLevels = 1;
    if (m_Spec.generateMips) {
        uint32_t dim = std::max(m_Spec.width, m_Spec.height);
        while (dim > 1) { dim >>= 1; ++mipLevels; }
    }

    const auto [internalFmt, baseFmt, dataType] = GetFormatInfo(m_Spec.format);

    if (is3D) {
        glCreateTextures(GL_TEXTURE_3D, 1, &m_TextureID);
        glTextureStorage3D(m_TextureID, mipLevels, internalFmt,
                           static_cast<GLsizei>(m_Spec.width),
                           static_cast<GLsizei>(m_Spec.height),
                           static_cast<GLsizei>(m_Spec.depth));
    } else {
        glCreateTextures(GL_TEXTURE_2D, 1, &m_TextureID);
        glTextureStorage2D(m_TextureID, mipLevels, internalFmt,
                           static_cast<GLsizei>(m_Spec.width),
                           static_cast<GLsizei>(m_Spec.height));
    }

    ORB_ASSERT(m_TextureID != 0, "glCreateTextures failed");

    // Set sampling parameters
    glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(ToGL(m_Spec.minFilter)));
    glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(ToGL(m_Spec.magFilter)));
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S,     static_cast<GLint>(ToGL(m_Spec.wrapS)));
    glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T,     static_cast<GLint>(ToGL(m_Spec.wrapT)));

    ORB_CORE_TRACE("GLTexture created {}x{}x{} (id={}, fmt={})",
                   m_Spec.width, m_Spec.height, m_Spec.depth,
                   m_TextureID, static_cast<uint32_t>(m_Spec.format));
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

GLTexture::GLTexture(const TextureSpec& spec)
    : m_Spec(spec)
{
    AllocateStorage();
}

GLTexture::GLTexture(const TextureSpec& spec, const void* data)
    : m_Spec(spec)
{
    AllocateStorage();
    if (m_Spec.depth > 1)
        Upload3D(data);
    else
        Upload(data, 0);
}

GLTexture::~GLTexture()
{
    if (m_TextureID) {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }
}

GLTexture::GLTexture(GLTexture&& other) noexcept
    : m_Spec(other.m_Spec)
    , m_TextureID(other.m_TextureID)
{
    other.m_TextureID = 0;
}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept
{
    if (this != &other) {
        if (m_TextureID) glDeleteTextures(1, &m_TextureID);
        m_Spec        = other.m_Spec;
        m_TextureID   = other.m_TextureID;
        other.m_TextureID = 0;
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
// Data upload
// ─────────────────────────────────────────────────────────────────────────────

void GLTexture::Upload(const void* data, uint32_t mipLevel)
{
    ORB_ASSERT(m_TextureID != 0, "GLTexture::Upload called on invalid texture");

    const auto [internalFmt, baseFmt, dataType] = GetFormatInfo(m_Spec.format);

    const uint32_t w = std::max(1u, m_Spec.width  >> mipLevel);
    const uint32_t h = std::max(1u, m_Spec.height >> mipLevel);

    glTextureSubImage2D(m_TextureID,
                        static_cast<GLint>(mipLevel),
                        0, 0,
                        static_cast<GLsizei>(w),
                        static_cast<GLsizei>(h),
                        baseFmt, dataType, data);

    if (m_Spec.generateMips && mipLevel == 0)
        glGenerateTextureMipmap(m_TextureID);
}

void GLTexture::Upload3D(const void* data)
{
    ORB_ASSERT(m_TextureID != 0, "GLTexture::Upload3D called on invalid texture");
    ORB_ASSERT(m_Spec.depth > 1, "GLTexture::Upload3D called on a 2D texture");

    const auto [internalFmt, baseFmt, dataType] = GetFormatInfo(m_Spec.format);

    glTextureSubImage3D(m_TextureID,
                        0, // mip level
                        0, 0, 0,
                        static_cast<GLsizei>(m_Spec.width),
                        static_cast<GLsizei>(m_Spec.height),
                        static_cast<GLsizei>(m_Spec.depth),
                        baseFmt, dataType, data);

    if (m_Spec.generateMips)
        glGenerateTextureMipmap(m_TextureID);
}

// ─────────────────────────────────────────────────────────────────────────────
// Binding
// ─────────────────────────────────────────────────────────────────────────────

void GLTexture::Bind(uint32_t slot) const noexcept
{
    glBindTextureUnit(slot, m_TextureID);
}

void GLTexture::Unbind(uint32_t slot) noexcept
{
    glBindTextureUnit(slot, 0);
}

} // namespace Orbital
