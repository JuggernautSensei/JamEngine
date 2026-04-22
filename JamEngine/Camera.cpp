#include "pch.h"

#include "Camera.h"

namespace Jam
{

const Mtx4& Camera::GetProj() const
{
    if (m_bProjDirty)
    {
        if (m_projection == eProjection::Orthographic)
        {
            m_projCache = Mtx4::CreateOrtho(m_width, m_height, m_nearZ, m_farZ);
        }
        else
        {
            const float ar = GetAspectRatio();
            m_projCache    = Mtx4::CreatePerspectiveFov(m_fovYRad, ar, m_nearZ, m_farZ);
        }

        m_bProjDirty = false;
    }

    return m_projCache;
}

void Camera::SetProjection(
    const eProjection _projection)
{
    if (m_projection == _projection)
    {
        return;
    }

    m_projection = _projection;
    m_bProjDirty = true;
}

void Camera::SetNearZ(
    const float _nearZ)
{
    m_nearZ      = _nearZ;
    m_nearZ      = Clamp(m_nearZ, kMinZ, m_farZ - 0.001f);
    m_bProjDirty = true;
}

void Camera::SetFarZ(
    const float _farZ)
{
    m_farZ       = _farZ;
    m_farZ       = Clamp(m_farZ, m_nearZ + 0.001f, kMaxZ);
    m_bProjDirty = true;
}

void Camera::SetFovYRad(
    const float _fovYRad)
{
    m_fovYRad    = _fovYRad;
    m_fovYRad    = Clamp(m_fovYRad, kMinFovYRad, kMaxFovYRad);
    m_bProjDirty = true;
}

void Camera::SetWidth(
    const float _width)
{
    m_width      = _width;
    m_width      = Clamp(m_width, kMinWidthHeight, kMaxWidthHeight);
    m_bProjDirty = true;
}

void Camera::SetHeight(
    const float _height)
{
    m_height     = _height;
    m_height     = Clamp(m_height, kMinWidthHeight, kMaxWidthHeight);
    m_bProjDirty = true;
}

void Camera::SetAspectRatio(
    const float _aspect)
{
    m_width  = 1.f;
    m_height = _aspect;
}

eProjection Camera::GetProjection() const
{
    return m_projection;
}

float Camera::GetAspectRatio() const
{
    return m_height / m_width;
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

bool Camera::IsDirty() const
{
    return m_bProjDirty;
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