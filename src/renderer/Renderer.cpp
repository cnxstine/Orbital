#include "renderer/Renderer.hpp"
#include "camera/Camera.hpp"
#include "core/Log.hpp"
#include "core/Time.hpp"

#include <glad/gl.h>

namespace Orbital {

Renderer::Renderer()
{
    ORB_CORE_TRACE("Renderer constructed");
}

void Renderer::Init(uint32_t viewportWidth, uint32_t viewportHeight)
{
    m_ViewportWidth  = viewportWidth;
    m_ViewportHeight = viewportHeight;

    ORB_CORE_INFO("Renderer initialised ({}x{})", viewportWidth, viewportHeight);
}

void Renderer::Shutdown()
{
    ORB_CORE_INFO("Renderer: total frames rendered: {}", Time::Frame());
}

// ─────────────────────────────────────────────────────────────────────────────

void Renderer::BeginFrame(const Camera& camera)
{
    m_FrameStart = Time::Elapsed();

    // Reset per-frame stats
    m_Stats = {};

    // Viewport + clear
    glViewport(0, 0,
        static_cast<GLsizei>(m_ViewportWidth),
        static_cast<GLsizei>(m_ViewportHeight));

    glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Upload camera data to UBO binding 0
    m_Context.BeginFrame(m_ViewportWidth, m_ViewportHeight, Time::Elapsed());
    m_Context.SetCamera(
        camera.GetViewMatrix(),
        camera.GetProjectionMatrix(),
        camera.GetPosition(),
        camera.GetNearPlane(),
        camera.GetFarPlane()
    );
    m_Context.UploadCameraUBO();
}

void Renderer::EndFrame()
{
    // Execute all queued draw commands
    m_CommandBuffer.Execute(m_Context);

    // Collect stats
    m_Stats.DrawCalls  = m_Context.GetDrawCallCount();
    m_Stats.FrameTimeMs = (Time::Elapsed() - m_FrameStart) * 1000.0f;

    m_Context.EndFrame();
    m_CommandBuffer.Clear();
}

// ─────────────────────────────────────────────────────────────────────────────

void Renderer::Submit(const DrawCommand& cmd)
{
    m_CommandBuffer.Submit(cmd);
}

void Renderer::SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    glViewport(static_cast<GLint>(x), static_cast<GLint>(y),
               static_cast<GLsizei>(w), static_cast<GLsizei>(h));
}

void Renderer::SetClearColor(const glm::vec4& color)
{
    m_ClearColor = color;
}

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) return; // Minimised window — skip
    m_ViewportWidth  = width;
    m_ViewportHeight = height;
    ORB_CORE_TRACE("Renderer: viewport resized to {}x{}", width, height);
}

} // namespace Orbital
