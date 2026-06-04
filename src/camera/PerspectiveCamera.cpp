#include "camera/PerspectiveCamera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace Orbital {

PerspectiveCamera::PerspectiveCamera(float fovRadians, float aspect, float near, float far)
    : m_FOV(fovRadians), m_Aspect(aspect), m_Near(near), m_Far(far)
{}

// ── Setters ──────────────────────────────────────────────────────────────────

void PerspectiveCamera::SetPosition(const glm::vec3& pos)
{
    m_Position   = pos;
    m_DirtyView  = true;
}

void PerspectiveCamera::SetTarget(const glm::vec3& target)
{
    m_Target    = target;
    m_DirtyView = true;
}

void PerspectiveCamera::SetUp(const glm::vec3& up)
{
    m_Up        = up;
    m_DirtyView = true;
}

void PerspectiveCamera::SetFOV(float radians)
{
    m_FOV       = radians;
    m_DirtyProj = true;
}

void PerspectiveCamera::SetAspect(float aspect)
{
    m_Aspect    = aspect;
    m_DirtyProj = true;
}

void PerspectiveCamera::SetNearFar(float near_, float far_)
{
    m_Near      = near_;
    m_Far       = far_;
    m_DirtyProj = true;
}

// ── Camera overrides ──────────────────────────────────────────────────────────

glm::mat4 PerspectiveCamera::GetViewMatrix() const
{
    if (m_DirtyView) {
        m_View      = glm::lookAt(m_Position, m_Target, m_Up);
        m_DirtyView = false;
    }
    return m_View;
}

glm::mat4 PerspectiveCamera::GetProjectionMatrix() const
{
    if (m_DirtyProj) {
        m_Proj      = glm::perspective(m_FOV, m_Aspect, m_Near, m_Far);
        m_DirtyProj = false;
    }
    return m_Proj;
}

glm::vec3 PerspectiveCamera::GetForward() const
{
    return glm::normalize(m_Target - m_Position);
}

glm::vec3 PerspectiveCamera::GetRight() const
{
    return glm::normalize(glm::cross(GetForward(), m_Up));
}

void PerspectiveCamera::RecomputeMatrices() const
{
    if (m_DirtyView) {
        m_View      = glm::lookAt(m_Position, m_Target, m_Up);
        m_DirtyView = false;
    }
    if (m_DirtyProj) {
        m_Proj      = glm::perspective(m_FOV, m_Aspect, m_Near, m_Far);
        m_DirtyProj = false;
    }
}

} // namespace Orbital
