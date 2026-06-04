#pragma once

/**
 * @file renderer/RenderContext.hpp
 * @brief Per-frame transient render state and camera UBO management.
 *
 * RenderContext is owned by the Renderer and lives for the lifetime
 * of the application. BeginFrame()/EndFrame() bracket each render tick.
 *
 * CameraUBO layout matches the GLSL std140 binding 0 block defined in
 * assets/shaders/common/uniforms.glsl.  The C++ struct is explicitly
 * padded to guarantee layout compatibility.
 *
 * Usage:
 *   ctx.BeginFrame(width, height, time);
 *   ctx.SetCamera(view, proj, eye, near, far);
 *   ctx.UploadCameraUBO();
 *   // ... submit draw calls ...
 *   ctx.EndFrame();
 */

#include "renderer/backend/GLBuffer.hpp"

#include <glm/glm.hpp>
#include <cstdint>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// CameraData — mirrors GLSL layout(std140, binding=0) uniform CameraUBO
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Camera uniform data, std140-compatible layout.
 *
 * Each vec3 is padded to vec4 per std140 rules.
 * The struct is packed in the order it appears in uniforms.glsl.
 */
struct alignas(16) CameraData {
    glm::mat4 view        {1.0f};   ///< 64 bytes
    glm::mat4 proj        {1.0f};   ///< 64 bytes
    glm::mat4 viewProj    {1.0f};   ///< 64 bytes
    glm::mat4 invViewProj {1.0f};   ///< 64 bytes

    glm::vec3 eye   {0.0f};         ///< 12 bytes
    float     near_ = 0.01f;        ///<  4 bytes  (pads eye to 16)

    glm::vec3 forward {0,0,-1};     ///< 12 bytes
    float     far_   = 1000.0f;     ///<  4 bytes  (pads forward to 16)

    glm::vec2 resolution {1280,720};///< 8 bytes
    float     time   = 0.0f;        ///< 4 bytes
    float     _pad   = 0.0f;        ///< 4 bytes  (pads to 16)
};

// 4 mat4 = 256, eye+near = 16, fwd+far = 16, res+time+pad = 16 → 304 bytes
static_assert(sizeof(CameraData) == 304,
              "CameraData layout mismatch — check std140 padding");

// ─────────────────────────────────────────────────────────────────────────────

class RenderContext {
public:
    RenderContext();
    ~RenderContext() = default;

    RenderContext(const RenderContext&)            = delete;
    RenderContext& operator=(const RenderContext&) = delete;

    // ── Frame lifecycle ───────────────────────────────────────────────────────

    /**
     * @brief Begin a frame: update viewport size, clear draw-call counter.
     * @param width   Current viewport width in pixels.
     * @param height  Current viewport height in pixels.
     * @param time    Elapsed time in seconds (for u_Time shader uniform).
     */
    void BeginFrame(uint32_t width, uint32_t height, float time);

    /**
     * @brief End the frame (currently a no-op, reserved for future use).
     */
    void EndFrame();

    // ── Camera ────────────────────────────────────────────────────────────────

    /**
     * @brief Set the active camera matrices and parameters.
     */
    void SetCamera(const glm::mat4& view,
                   const glm::mat4& proj,
                   glm::vec3        eye,
                   float            nearPlane,
                   float            farPlane);

    /**
     * @brief Upload the current CameraData struct to the UBO and bind it to
     *        binding point 0 (matching GLSL layout(std140, binding=0)).
     */
    void UploadCameraUBO();

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] const GLBuffer&    GetCameraUBO()    const noexcept { return m_CameraUBO; }
    [[nodiscard]] const CameraData&  GetCameraData()   const noexcept { return m_CameraData; }
    [[nodiscard]] uint32_t           GetDrawCallCount()const noexcept { return m_DrawCalls;  }
    [[nodiscard]] uint32_t           GetViewportWidth()const noexcept { return m_Width;      }
    [[nodiscard]] uint32_t           GetViewportHeight()const noexcept{ return m_Height;     }

    /// Called by CommandBuffer when a draw call is executed.
    void IncrementDrawCalls() noexcept { ++m_DrawCalls; }

private:
    GLBuffer    m_CameraUBO;
    CameraData  m_CameraData;

    uint32_t    m_Width     = 1280;
    uint32_t    m_Height    = 720;
    uint32_t    m_DrawCalls = 0;
};

} // namespace Orbital
