#pragma once

/**
 * @file renderer/backend/GLTexture.hpp
 * @brief RAII wrapper around OpenGL texture objects (2D and 3D).
 *
 * Uses OpenGL 4.5 DSA:
 *   glCreateTextures
 *   glTextureStorage2D / glTextureStorage3D
 *   glTextureSubImage2D / glTextureSubImage3D
 *   glGenerateTextureMipmap
 *   glBindTextureUnit
 *   glTextureParameteri
 *
 * Move-only; the destructor calls glDeleteTextures.
 *
 * Usage:
 *   TextureSpec spec;
 *   spec.width  = 1024;
 *   spec.height = 1024;
 *   spec.format = TextureFormat::RGBA8;
 *   spec.generateMips = true;
 *   GLTexture tex(spec, pixelData);
 *   tex.Bind(0);
 */

#include <cstdint>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────

/// Internal storage format for texture data.
enum class TextureFormat : uint32_t {
    R8,          ///< GL_R8       — single channel, 8-bit unorm
    RG8,         ///< GL_RG8      — two channels, 8-bit unorm
    RGB8,        ///< GL_RGB8     — three channels, 8-bit unorm
    RGBA8,       ///< GL_RGBA8    — four channels, 8-bit unorm
    R32F,        ///< GL_R32F     — single channel, 32-bit float
    RGB16F,      ///< GL_RGB16F   — three channels, 16-bit float
    RGBA16F,     ///< GL_RGBA16F  — four channels, 16-bit float
    RGBA32F,     ///< GL_RGBA32F  — four channels, 32-bit float
    Depth32F,    ///< GL_DEPTH_COMPONENT32F — depth attachment
};

/// Sampling / interpolation filter.
enum class TextureFilter : uint32_t {
    Nearest,            ///< GL_NEAREST
    Linear,             ///< GL_LINEAR
    LinearMipmapLinear, ///< GL_LINEAR_MIPMAP_LINEAR (trilinear)
};

/// Texture coordinate wrapping mode.
enum class TextureWrap : uint32_t {
    Repeat,       ///< GL_REPEAT
    ClampToEdge,  ///< GL_CLAMP_TO_EDGE
    ClampToBorder,///< GL_CLAMP_TO_BORDER
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Complete specification for creating a texture object.
 */
struct TextureSpec {
    uint32_t      width        = 1;
    uint32_t      height       = 1;
    uint32_t      depth        = 1;     ///< 1 for 2D textures; >1 for 3D
    TextureFormat format       = TextureFormat::RGBA8;
    TextureFilter minFilter    = TextureFilter::Linear;
    TextureFilter magFilter    = TextureFilter::Linear;
    TextureWrap   wrapS        = TextureWrap::ClampToEdge;
    TextureWrap   wrapT        = TextureWrap::ClampToEdge;
    bool          generateMips = false;
};

// ─────────────────────────────────────────────────────────────────────────────

class GLTexture {
public:
    // ── Construction / destruction ─────────────────────────────────────────────

    /**
     * @brief Allocate texture storage without uploading pixel data.
     * @param spec  Texture specification.
     */
    explicit GLTexture(const TextureSpec& spec);

    /**
     * @brief Allocate texture storage and immediately upload pixel data.
     * @param spec  Texture specification.
     * @param data  Pointer to pixel data; must be non-null.
     */
    GLTexture(const TextureSpec& spec, const void* data);

    ~GLTexture();

    // Move-only
    GLTexture(GLTexture&& other) noexcept;
    GLTexture& operator=(GLTexture&& other) noexcept;

    GLTexture(const GLTexture&)            = delete;
    GLTexture& operator=(const GLTexture&) = delete;

    // ── Data upload ───────────────────────────────────────────────────────────

    /**
     * @brief Upload pixel data for a given mip level (2D textures).
     * @param data      Pixel data matching the texture's format.
     * @param mipLevel  Target mip level (0 = base).
     */
    void Upload(const void* data, uint32_t mipLevel = 0);

    /**
     * @brief Upload pixel data for a 3D texture.
     * @param data  Pixel data for all slices.
     */
    void Upload3D(const void* data);

    // ── Binding ───────────────────────────────────────────────────────────────

    /**
     * @brief Bind this texture to a texture unit.
     * @param slot  Texture unit index (layout(binding=N) in GLSL).
     */
    void Bind(uint32_t slot = 0) const noexcept;

    /**
     * @brief Unbind the texture from a texture unit (binds name=0).
     * @param slot  Texture unit index to clear.
     */
    static void Unbind(uint32_t slot = 0) noexcept;

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] uint32_t      GetID()     const noexcept { return m_TextureID; }
    [[nodiscard]] uint32_t      GetWidth()  const noexcept { return m_Spec.width; }
    [[nodiscard]] uint32_t      GetHeight() const noexcept { return m_Spec.height; }
    [[nodiscard]] TextureFormat GetFormat() const noexcept { return m_Spec.format; }
    [[nodiscard]] const TextureSpec& GetSpec() const noexcept { return m_Spec; }
    [[nodiscard]] bool          IsValid()   const noexcept { return m_TextureID != 0; }

private:
    void AllocateStorage();

    TextureSpec m_Spec;
    uint32_t    m_TextureID = 0;
};

} // namespace Orbital
