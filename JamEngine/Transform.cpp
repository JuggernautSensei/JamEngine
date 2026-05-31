#include "pch.h"

#include "Math.h"
#include "Transform.h"

namespace Jam
{

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

Mtx4 Transform::GetSRT() const
{
    if (m_dirtyFlags & eMatrixDirtyFlags::SRT)
    {
        m_cachedSRT = Mtx4::MakeSRT(m_scale, m_rotation, m_translation);
        m_dirtyFlags.Disable(eMatrixDirtyFlags::SRT);
    }
    return m_cachedSRT;
}

Mtx4 Transform::GetInvSRT() const
{
    if (m_dirtyFlags & eMatrixDirtyFlags::InvSRT)
    {
        m_cachedInvSRT = Mtx4::MakeInvSRT(m_scale, m_rotation, m_translation);
        m_dirtyFlags.Disable(eMatrixDirtyFlags::InvSRT);
    }
    return m_cachedInvSRT;
}

void Transform::SetSRT(const Mtx4& _srtMtx)
{
    _srtMtx.DecomposeSRT(&m_scale, &m_rotation, &m_translation);
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

Transform Transform::Inverse() const
{
    Transform ret     = {};
    ret.m_scale       = m_scale.RcpSafe();
    ret.m_rotation    = m_rotation.Conjugate();
    ret.m_translation = ret.m_rotation.Rotate(-m_translation) * ret.m_scale;

    return ret;
}

Transform Transform::Mult(const Transform& _trans) const
{
    Transform ret     = {};
    ret.m_translation = _trans.m_rotation.Rotate(m_translation * _trans.m_scale) + _trans.m_translation;
    ret.m_rotation    = m_rotation * _trans.m_rotation;
    ret.m_scale       = m_scale * _trans.m_scale;

    return ret;
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

void Transform::SetScale(const Vec3 _xyz)
{
    m_scale = _xyz;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetScale(float _x, float _y, float _z)
{
    SetScale({ _x, _y, _z });
}

void Transform::SetScale(const float _uniform)
{
    SetScale(_uniform, _uniform, _uniform);
}

void Transform::SetScaleX(const float _x)
{
    m_scale.x = _x;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetScaleY(const float _y)
{
    m_scale.y = _y;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetScaleZ(const float _z)
{
    m_scale.z = _z;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddScale(const Vec3 _xyz)
{
    SetScale(m_scale + _xyz);
}

void Transform::AddScale(float _x, float _y, float _z)
{
    AddScale({ _x, _y, _z });
}

void Transform::AddScale(const float _uniform)
{
    AddScale(_uniform, _uniform, _uniform);
}

void Transform::AddScaleX(const float _x)
{
    SetScaleX(m_scale.x + _x);
}

void Transform::AddScaleY(const float _y)
{
    SetScaleY(m_scale.y + _y);
}

void Transform::AddScaleZ(const float _z)
{
    SetScaleZ(m_scale.z + _z);
}

Quat Transform::GetRotation() const
{
    return m_rotation;
}

AxisAngle Transform::ToAxisAngle() const
{
    const float rad = 2.f * Acos(m_rotation.w);
    const float s   = Sqrt(1.f - m_rotation.w * m_rotation.w);

    if (Jam::IsZeroApprox(s))
    {
        return { Vec3::kUnitX, rad };
    }

    return { m_rotation.xyz / s, rad };
}

Vec3 Transform::ToEuler() const
{
    return m_rotation.ToEuler();
}

void Transform::SetRotation(const Quat _q)
{
    m_rotation = _q.Normalized();
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetRotation(const Vec3 _axis, const float _rad)
{
    SetRotation(Quat::FromAxisAngle(_axis, _rad));
}

void Transform::SetRotation(const Vec3 _eulerRad)
{
    SetRotation(Quat::FromEuler(_eulerRad));
}

void Transform::SetRotation(const float _pitchRad, const float _yawRad, const float _rollRad)
{
    SetRotation(Quat::FromEuler({ _pitchRad, _yawRad, _rollRad }));
}

void Transform::SetLookAt(const Vec3 _point, const Vec3 _u)
{
    SetLookTo((_point - m_translation).Normalized(), _u);
}

void Transform::SetLookTo(const Vec3 _f, const Vec3 _u)
{
    const Vec3 f = _f.Normalized();
    const Vec3 r = _u.Cross(f).Normalized();
    const Vec3 u = f.Cross(r).Normalized();

    m_rotation = Quat::FromRotationMatrix({
        { r, 0.f },
        { u, 0.f },
        { f, 0.f },
        Vec4::kUnitW
    });

    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddRotation(const Quat _q)
{
    m_rotation = (m_rotation * _q).Normalized();
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddRotation(const Vec3 _axis, const float _rad)
{
    AddRotation(Quat::FromAxisAngle(_axis, _rad));
}

void Transform::AddRotation(const Vec3 _eulerRad)
{
    AddRotation(Quat::FromEuler(_eulerRad));
}

void Transform::AddRotation(const float _pitchRad, const float _yawRad, const float _rollRad)
{
    AddRotation(Quat::FromEuler({ _pitchRad, _yawRad, _rollRad }));
}

void Transform::AddRotationPitch(const float _rad)
{
    AddRotation(_rad, 0.f, 0.f);
}

void Transform::AddRotationYaw(const float _rad)
{
    AddRotation(0.f, _rad, 0.f);
}

void Transform::AddRotationRoll(const float _rad)
{
    AddRotation(0.f, 0.f, _rad);
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

void Transform::SetTranslation(const Vec3 _xyz)
{
    m_translation = _xyz;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetTranslation(float _x, float _y, float _z)
{
    SetTranslation({ _x, _y, _z });
}

void Transform::SetTranslationX(const float _x)
{
    m_translation.x = _x;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetTranslationY(const float _y)
{
    m_translation.y = _y;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::SetTranslationZ(const float _z)
{
    m_translation.z = _z;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddTranslation(const Vec3 _xyz)
{
    m_translation += _xyz;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddTranslation(float _x, float _y, float _z)
{
    AddTranslation({ _x, _y, _z });
}

void Transform::AddTranslationX(const float _x)
{
    m_translation.x += _x;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddTranslationY(const float _y)
{
    m_translation.y += _y;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

void Transform::AddTranslationZ(const float _z)
{
    m_translation.z += _z;
    m_dirtyFlags |= eMatrixDirtyFlags::All;
}

}   // namespace Jam
