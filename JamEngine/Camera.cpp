#include "pch.h"

#include "Camera.h"

namespace Jam
{

const Mtx4& Camera::GetProj() const
{
    if (m_dirtyFlags & eMatrixDirtyFlags::Proj)
    {
        if (m_projection == eProjection::Orthographic)
        {
            m_cachedProj = Mtx4::MakeOrtho(m_width, m_height, m_nearZ, m_farZ);
        }
        else
        {
            const float ar = GetAspectRatio();
            m_cachedProj   = Mtx4::MakePerspective(m_fovYRad, ar, m_nearZ, m_farZ);
        }
        m_dirtyFlags.Disable(eMatrixDirtyFlags::Proj);
    }
    return m_cachedProj;
}

const Mtx4& Camera::GetInvProj() const
{
    if (m_dirtyFlags & eMatrixDirtyFlags::InvProj)
    {
        if (m_projection == eProjection::Orthographic)
        {
            m_cachedInvProj = Mtx4::MakeInvOrtho(m_width, m_height, m_nearZ, m_farZ);
        }
        else
        {
            const float ar  = GetAspectRatio();
            m_cachedInvProj = Mtx4::MakeInvPerspective(m_fovYRad, ar, m_nearZ, m_farZ);
        }
        m_dirtyFlags.Disable(eMatrixDirtyFlags::InvProj);
    }
    return m_cachedInvProj;
}

void Camera::SetProjection(
    const eProjection _projection)
{
    m_projection = _projection;
    m_dirtyFlags = eMatrixDirtyFlags::All;
}

void Camera::SetNearZ(
    const float _nearZ)
{
    m_nearZ      = _nearZ;
    m_nearZ      = Clamp(m_nearZ, kMinZ, m_farZ - 0.001f);
    m_dirtyFlags = eMatrixDirtyFlags::All;
}

void Camera::SetFarZ(
    const float _farZ)
{
    m_farZ       = _farZ;
    m_farZ       = Clamp(m_farZ, m_nearZ + 0.001f, kMaxZ);
    m_dirtyFlags = eMatrixDirtyFlags::All;
}

void Camera::SetFovYRad(
    const float _fovYRad)
{
    m_fovYRad    = _fovYRad;
    m_fovYRad    = Clamp(m_fovYRad, kMinFovYRad, kMaxFovYRad);
    m_dirtyFlags = eMatrixDirtyFlags::All;
}

void Camera::SetWidth(
    const float _width)
{
    m_width      = _width;
    m_width      = Clamp(m_width, kMinWidthHeight, kMaxWidthHeight);
    m_dirtyFlags = eMatrixDirtyFlags::All;
}

void Camera::SetHeight(
    const float _height)
{
    m_height     = _height;
    m_height     = Clamp(m_height, kMinWidthHeight, kMaxWidthHeight);
    m_dirtyFlags = eMatrixDirtyFlags::All;
}

void Camera::SetAspectRatio(
    const float _aspect)
{
    m_width      = 1.f;
    m_height     = _aspect;
    m_dirtyFlags = eMatrixDirtyFlags::All;
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