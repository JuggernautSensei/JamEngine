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
    return Div(m_width, m_height);
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

void Camera::SetProjection(
    const eProjection _projection)
{
    m_projection = _projection;
}

void Camera::SetNearZ(
    const float _nearZ)
{
    m_nearZ = _nearZ;
    m_nearZ = Clamp(m_nearZ, k_minZ, m_farZ - k_epsilonZ);
}

void Camera::SetFarZ(
    const float _farZ)
{
    m_farZ = _farZ;
    m_farZ = Clamp(m_farZ, m_nearZ + k_epsilonZ, k_maxZ);
}

void Camera::SetFovYRad(
    const float _fovYRad)
{
    m_fovYRad = _fovYRad;
    m_fovYRad = Clamp(m_fovYRad, k_minFovYRad, k_maxFovYRad);
}

void Camera::SetWidth(
    const float _width)
{
    JAM_ASSERT(_width > 0.f, "Camera width must be greater than 0.");
    m_width = _width;
    m_width = Clamp(m_width, k_minWidthHeight, k_maxWidthHeight);
}

void Camera::SetHeight(
    const float _height)
{
    JAM_ASSERT(_height > 0.f, "Camera height must be greater than 0.");
    m_height = _height;
    m_height = Clamp(m_height, k_minWidthHeight, k_maxWidthHeight);
}

Mat4 Camera::ToProjMatrix() const
{
    if (m_projection == eProjection::Orthographic)
    {
        return CreateOrthoMatrix(m_width, m_height, m_nearZ, m_farZ);
    }
    else
    {
        const float ar = GetAspectRatio();
        return CreatePerspectiveMatrix(m_fovYRad, ar, m_nearZ, m_farZ);
    }
}

}   // namespace jam