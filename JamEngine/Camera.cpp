#include "pch.h"

#include "Camera.h"

namespace jam
{

Camera::Camera() = default;

eProjection Camera::GetProjection() const
{
    return m_projection;
}

float Camera::GetAspectRatio() const
{
    return static_cast<float>(m_width) / static_cast<float>(m_height);
}

float Camera::GetNearZ() const
{
    return m_nearZ;
}

float Camera::GetFarZ() const
{
    return m_farZ;
}

float Camera::GetFovYRad() const
{
    return m_fovYRad;
}

float Camera::GetWidth() const
{
    return m_width;
}

float Camera::GetHeight() const
{
    return m_height;
}

void Camera::SetProjection(const eProjection _projection)
{
    m_projection = _projection;
}

void Camera::SetNearZ(const float _nearZ)
{
    m_nearZ = _nearZ;
    m_nearZ = Clamp(m_nearZ, k_nearestZ, m_farZ - k_nearFarEpsilon);
}

void Camera::SetFarZ(const float _farZ)
{
    m_farZ = _farZ;
    m_farZ = Clamp(m_farZ, m_nearZ + k_nearFarEpsilon, k_farthestZ);
}

void Camera::SetFovYRad(const float _fovYRad)
{
    m_fovYRad = _fovYRad;
}

void Camera::SetWidth(const float _width)
{
    JAM_ASSERT(_width > 0.f, "Camera width must be greater than 0.");
    m_width = _width;
}

void Camera::SetHeight(const float _height)
{
    JAM_ASSERT(_height > 0.f, "Camera height must be greater than 0.");
    m_height = _height;
}

Mat4 Camera::CreateProjMatrix() const
{
    if (m_projection == eProjection::Orthographic)
        return CreateOrthographic(GetAspectRatio(), 1.f, m_nearZ, m_farZ);
    else
        return CreatePerspectiveFov(m_fovYRad, GetAspectRatio(), m_nearZ, m_farZ);
}

}   // namespace jam