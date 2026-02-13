#include "pch.h"

#include "Camera.h"

namespace Jam
{

Mtx4 Camera::ToProjMatrix() const
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

void Camera::SetProjection(
    const eProjection _projection)
{
    m_projection = _projection;
}

void Camera::SetNearZ(
    const float _nearZ)
{
    m_nearZ = _nearZ;
    m_nearZ = Clamp(m_nearZ, k_minZ, m_farZ - 0.001f);
}

void Camera::SetFarZ(
    const float _farZ)
{
    m_farZ = _farZ;
    m_farZ = Clamp(m_farZ, m_nearZ + 0.001f, k_maxZ);
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
    m_width = _width;
    m_width = Clamp(m_width, k_minWidthHeight, k_maxWidthHeight);
}

void Camera::SetHeight(
    const float _height)
{
    m_height = _height;
    m_height = Clamp(m_height, k_minWidthHeight, k_maxWidthHeight);
}

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

void Camera::WriteToArchive(
    ArchiveWriter& _writer) const
{
    _writer.WriteField("projection", m_projection);
    _writer.WriteField("nearZ", m_nearZ);
    _writer.WriteField("farZ", m_farZ);
    _writer.WriteField("fovYRad", m_fovYRad);
    _writer.WriteField("width", m_width);
    _writer.WriteField("height", m_height);
}

Error Camera::ReadFromArchive(
    ArchiveReader& _reader)
{
    eProjection projValue = {};
    float       fValue    = 0.f;

    if (const Error err = _reader.ReadField("projection", projValue))
    {
        return err;
    }
    SetProjection(projValue);

    if (const Error err = _reader.ReadField("nearZ", fValue))
    {
        return err;
    }
    SetNearZ(fValue);

    if (const Error err = _reader.ReadField("farZ", fValue))
    {
        return err;
    }
    SetFarZ(fValue);

    if (const Error err = _reader.ReadField("fovYRad", fValue))
    {
        return err;
    }
    SetFovYRad(fValue);

    if (const Error err = _reader.ReadField("width", fValue))
    {
        return err;
    }
    SetWidth(fValue);

    if (const Error err = _reader.ReadField("height", fValue))
    {
        return err;
    }
    SetHeight(fValue);

    return Error::None;
}

}   // namespace Jam