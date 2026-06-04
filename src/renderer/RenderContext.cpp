#include "renderer/RenderContext.hpp"

#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

RenderContext::RenderContext()
    : m_CameraUBO(BufferTarget::Uniform, BufferUsage::DynamicDraw)
{
    // Allocate UBO storage upfront (no data yet)
    m_CameraUBO.Upload(nullptr, sizeof(CameraData));
    // Bind to binding point 0 for the lifetime of the context
    m_CameraUBO.BindBase(0);

    ORB_CORE_INFO("RenderContext initialised (UBO id={}, size={} bytes)",
                  m_CameraUBO.GetID(), sizeof(CameraData));
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void RenderContext::BeginFrame(uint32_t width, uint32_t height, float time)
{
    m_Width     = width;
    m_Height    = height;
    m_DrawCalls = 0;

    m_CameraData.resolution = glm::vec2(static_cast<float>(width),
                                        static_cast<float>(height));
    m_CameraData.time = time;
}

void RenderContext::EndFrame()
{
    // Reserved for future per-frame GL state cleanup
}

// ─────────────────────────────────────────────────────────────────────────────
// Camera
// ─────────────────────────────────────────────────────────────────────────────

void RenderContext::SetCamera(const glm::mat4& view,
                               const glm::mat4& proj,
                               glm::vec3        eye,
                               float            nearPlane,
                               float            farPlane)
{
    m_CameraData.view        = view;
    m_CameraData.proj        = proj;
    m_CameraData.viewProj    = proj * view;
    m_CameraData.invViewProj = glm::inverse(m_CameraData.viewProj);

    m_CameraData.eye   = eye;
    m_CameraData.near_ = nearPlane;
    m_CameraData.far_  = farPlane;

    // Extract forward direction from the view matrix (third row, negated for look-at)
    m_CameraData.forward = glm::normalize(
        glm::vec3(-view[0][2], -view[1][2], -view[2][2]));
}

void RenderContext::UploadCameraUBO()
{
    m_CameraUBO.UploadSubData(&m_CameraData, 0, sizeof(CameraData));
    m_CameraUBO.BindBase(0); // Ensure binding point 0 is always live
}

} // namespace Orbital
