#include "pch.h"

#include "Transform.h"

namespace Jam
{

Transform::Transform(
    const Vec3 _scale,
    const Quat _rotation,
    const Vec3 _translation)
    : m_scale(_scale)
    , m_rotation(_rotation)
    , m_translation(_translation)
{
    JAM_ASSERT(_rotation.IsNormalized(), "Transform rotation must be normalized.");
}

Vec3 Transform::GetRight() const
{
    return m_rotation.Rotate(Vec3::kUnitX);
}

Vec3 Transform::GetLeft() const
{
    return m_rotation.Rotate(-Vec3::kUnitX);
}

Vec3 Transform::GetUp() const
{
    return m_rotation.Rotate(Vec3::kUnitY);
}

Vec3 Transform::GetDown() const
{
    return m_rotation.Rotate(-Vec3::kUnitY);
}

Vec3 Transform::GetForward() const
{
    return m_rotation.Rotate(Vec3::kUnitZ);
}

Vec3 Transform::GetBackward() const
{
    return m_rotation.Rotate(-Vec3::kUnitZ);
}

const Mtx4& Transform::GetSRT() const
{
    if (m_dirtyFlags & eMatrixDirtyFlags::SRT)
    {
        m_cachedSRT = Mtx4::MakeSRT(m_scale, m_rotation, m_translation);
        m_dirtyFlags.Disable(eMatrixDirtyFlags::SRT);
    }
    return m_cachedSRT;
}

const Mtx4& Transform::GetInvSRT() const
{
    if (m_dirtyFlags & eMatrixDirtyFlags::InvSRT)
    {
        m_cachedInvSRT = Mtx4::MakeInvSRT(m_scale, m_rotation, m_translation);
        m_dirtyFlags.Disable(eMatrixDirtyFlags::InvSRT);
    }
    return m_cachedInvSRT;
}

void Transform::SetSRT(
    const Mtx4& _srtMtx)
{
    _srtMtx.DecomposeSRT(&m_scale, &m_rotation, &m_translation);
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

Transform Transform::Inverse() const
{
    const Vec3 s = m_scale.RcpSafe();
    const Quat r = m_rotation.Conjugate();
    const Vec3 t = r.Rotate(-m_translation) * s;
    return { s, r, t };
}

Transform Transform::Mult(
    const Transform& _trans) const
{
    const Vec3 s = m_scale * _trans.m_scale;
    const Quat r = _trans.m_rotation * m_rotation;
    const Vec3 t = _trans.m_rotation.Rotate(m_translation * _trans.m_scale) + _trans.m_translation;
    return { s, r, t };
}

Vec3 Transform::GetScale() const
{
    return m_scale;
}

float Transform::GetScaleX() const
{
    return m_scale.x;
}

float Transform::GetScaleY() const
{
    return m_scale.y;
}

float Transform::GetScaleZ() const
{
    return m_scale.z;
}

void Transform::SetScale(
    const Vec3 _xyz)
{
    m_scale = _xyz;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetScale(
    const float _uniform)
{
    SetScale({ _uniform, _uniform, _uniform });
}

void Transform::SetScaleX(
    const float _x)
{
    m_scale.x = _x;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetScaleY(
    const float _y)
{
    m_scale.y = _y;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetScaleZ(
    const float _z)
{
    m_scale.z = _z;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddScale(
    const Vec3 _xyz)
{
    SetScale(m_scale + _xyz);
}

void Transform::AddScale(
    const float _uniform)
{
    AddScale(Vec3 { _uniform });
}

void Transform::AddScaleX(
    const float _x)
{
    SetScaleX(m_scale.x + _x);
}

void Transform::AddScaleY(
    const float _y)
{
    SetScaleY(m_scale.y + _y);
}

void Transform::AddScaleZ(
    const float _z)
{
    SetScaleZ(m_scale.z + _z);
}

Quat Transform::GetRotation() const
{
    return m_rotation;
}

AxisAngle Transform::ToAxisAngle() const
{
    return m_rotation.ToAxisAngle();
}

Vec3 Transform::ToEuler() const
{
    return m_rotation.ToEuler();
}

void Transform::SetRotation(
    const Quat _q)
{
    JAM_ASSERT(_q.IsNormalized(), "Transform rotation must be normalized.");

    m_rotation = _q;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetRotation(
    const Vec3  _axis,
    const float _rad)
{
    m_rotation = Quat::FromAxisAngle(_axis, _rad);
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetRotation(
    const Vec3 _euler)
{
    m_rotation = Quat::FromEuler(_euler);
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetLookAt(
    const Vec3 _focusPoint,
    const Vec3 _u)
{
    const Vec3 f = (_focusPoint - m_translation).Normalized();
    SetLookTo(f, _u);
}

void Transform::SetLookTo(
    const Vec3 _f,
    const Vec3 _u)
{
    m_rotation = Quat::FromLookTo(_f, _u);
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddRotation(
    const Quat _q)
{
    JAM_ASSERT(_q.IsNormalized(), "Transform rotation must be normalized.");

    m_rotation = (_q * m_rotation).Normalized();
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddRotation(
    const Vec3  _axis,
    const float _rad)
{
    const Quat q = Quat::FromAxisAngle(_axis, _rad);
    AddRotation(q);
}

void Transform::AddRotation(
    const Vec3 _euler)
{
    const Quat q = Quat::FromEuler(_euler);
    AddRotation(q);
}

void Transform::AddRotationPitch(
    const float _rad)
{
    AddRotation({ _rad, 0.f, 0.f });
}

void Transform::AddRotationYaw(
    const float _rad)
{
    AddRotation({ 0.f, _rad, 0.f });
}

void Transform::AddRotationRoll(
    const float _rad)
{
    AddRotation({ 0.f, 0.f, _rad });
}

Vec3 Transform::GetTranslation() const
{
    return m_translation;
}

float Transform::GetTranslationX() const
{
    return m_translation.x;
}

float Transform::GetTranslationY() const
{
    return m_translation.y;
}

float Transform::GetTranslationZ() const
{
    return m_translation.z;
}

void Transform::SetTranslation(
    const Vec3 _xyz)
{
    m_translation = _xyz;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetTranslationX(
    const float _x)
{
    m_translation.x = _x;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetTranslationY(
    const float _y)
{
    m_translation.y = _y;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetTranslationZ(
    const float _z)
{
    m_translation.z = _z;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddTranslation(
    const Vec3 _xyz)
{
    m_translation += _xyz;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddTranslationX(
    const float _x)
{
    m_translation.x += _x;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddTranslationY(
    const float _y)
{
    m_translation.y += _y;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddTranslationZ(
    const float _z)
{
    m_translation.z += _z;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

}   // namespace Jam
