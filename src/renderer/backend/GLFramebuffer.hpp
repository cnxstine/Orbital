#pragma once

/**
 * @file renderer/backend/GLFramebuffer.hpp
 * @brief RAII wrapper around an OpenGL Framebuffer Object (FBO).
 *
 * Supports multiple color attachments and a single depth attachment.
 * Uses GLTexture for all attachments (DSA-friendly).
 *
 * Workflow:
 *   1. Construct with dimensions.
 *   2. Add color / depth attachments.
 *   3. Call Build() — validates completeness via glCheckFramebufferStatus.
 *   4. Bind() before rendering, Unbind() when done.
 *   5. Call Resize() when the viewport changes — rebuilds all attachments.
 *
 * Usage:
 *   GLFramebuffer fbo(1280, 720);
 *   fbo.AddColorAttachment(TextureFormat::RGBA8);
 *   fbo.SetDepthAttachment(TextureFormat::Depth32F);
 *   bool ok = fbo.Build();
 *   fbo.Bind();
 *   // ... render ...
 *   fbo.Unbind();
 *   const GLTexture& color = fbo.GetColorAttachment(0);
 */

#include "renderer/backend/GLTexture.hpp"

#include <cstdint>
#include <vector>
#include <optional>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────

class GLFramebuffer {
public:
    // ── Construction / destruction ─────────────────────────────────────────────

    /**
     * @brief Create an unconfigured framebuffer of the given dimensions.
     * @param width   Viewport width in pixels.
     * @param height  Viewport height in pixels.
     */
    GLFramebuffer(uint32_t width, uint32_t height);
    ~GLFramebuffer();

    // Move-only
    GLFramebuffer(GLFramebuffer&& other) noexcept;
    GLFramebuffer& operator=(GLFramebuffer&& other) noexcept;

    GLFramebuffer(const GLFramebuffer&)            = delete;
    GLFramebuffer& operator=(const GLFramebuffer&) = delete;

    // ── Attachment configuration ──────────────────────────────────────────────

    /**
     * @brief Add a color attachment texture with the given format.
     * @param format  Pixel format for the color attachment.
     * @return Index of the new attachment (0-based, for GetColorAttachment).
     */
    uint32_t AddColorAttachment(TextureFormat format);

    /**
     * @brief Add a depth attachment texture.
     * @param format  Must be TextureFormat::Depth32F.
     */
    void SetDepthAttachment(TextureFormat format = TextureFormat::Depth32F);

    /**
     * @brief Finalize the framebuffer and check completeness.
     * @return true if glCheckFramebufferStatus returns GL_FRAMEBUFFER_COMPLETE.
     */
    [[nodiscard]] bool Build();

    // ── Binding ───────────────────────────────────────────────────────────────

    void Bind()   const noexcept;

    /// Bind the default framebuffer (id = 0).
    static void Unbind() noexcept;

    // ── Resize ────────────────────────────────────────────────────────────────

    /**
     * @brief Resize all attachments. Call Build() again after this.
     * @param w  New width.
     * @param h  New height.
     */
    void Resize(uint32_t w, uint32_t h);

    // ── Attachment access ─────────────────────────────────────────────────────

    /**
     * @brief Retrieve a color attachment texture by index.
     * @param index  Attachment index (0 = first color attachment).
     */
    [[nodiscard]] const GLTexture& GetColorAttachment(uint32_t index) const;

    /**
     * @brief Retrieve the depth attachment texture.
     * @pre  SetDepthAttachment() must have been called and Build() must have returned true.
     */
    [[nodiscard]] const GLTexture& GetDepthAttachment() const;

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] uint32_t GetID()     const noexcept { return m_FboID;  }
    [[nodiscard]] uint32_t GetWidth()  const noexcept { return m_Width;  }
    [[nodiscard]] uint32_t GetHeight() const noexcept { return m_Height; }
    [[nodiscard]] bool     IsValid()   const noexcept { return m_FboID != 0; }
    [[nodiscard]] bool     IsComplete()const noexcept { return m_Complete; }

private:
    /// Destroy the GL framebuffer object and clear the ID.
    void DestroyFBO();

    /// Attach all stored textures to the FBO.
    void AttachTextures();

    uint32_t m_FboID   = 0;
    uint32_t m_Width   = 0;
    uint32_t m_Height  = 0;
    bool     m_Complete = false;

    struct AttachmentSpec {
        TextureFormat format;
    };

    std::vector<AttachmentSpec>    m_ColorSpecs;
    std::optional<AttachmentSpec>  m_DepthSpec;

    std::vector<GLTexture>         m_ColorAttachments;
    std::optional<GLTexture>       m_DepthAttachment;
};

} // namespace Orbital
