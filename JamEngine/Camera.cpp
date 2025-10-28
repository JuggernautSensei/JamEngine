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
    return m_width / m_height;
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
    m_nearZ = Clamp(m_nearZ, k_nearestZ, m_farZ - k_epsilon);
}

void Camera::SetFarZ(const float _farZ)
{
    m_farZ = _farZ;
    m_farZ = Clamp(m_farZ, m_nearZ + k_epsilon, k_farthestZ);
}

void Camera::SetFovYRad(const float _fovYRad)
{
    m_fovYRad = _fovYRad;
}

void Camera::SetWidth(const float _width)
{
    m_width = _width;
}

void Camera::SetHeight(const float _height)
{
    m_height = _height;
}

void Camera::SetAspectRatio(const eAspectRatio _aspectRatio)
{
    m_height = 1.f;
    switch (_aspectRatio)
    {
        case eAspectRatio::Ratio_4_3:
            m_width = 4.f / 3.f;
            break;

        case eAspectRatio::Ratio_16_9:
            m_width = 16.f / 9.f;
            break;

        case eAspectRatio::Ratio_21_9:
            m_width = 21.f / 9.f;
            break;

        default:
            JAM_ASSERT(0, "Unknown aspect ratio");
            break;
    }
}

Mat4 Camera::CreateProj() const
{
    if (m_projection == eProjection::Orthographic)
    {
        return CreateOrthographic(m_width, m_height, m_nearZ, m_farZ);
    }
    else
    {
        return CreatePerspectiveFov(m_fovYRad, m_width / m_height, m_nearZ, m_farZ);
    }
}

}   // namespace jam