#include "pch.h"

#include "Math.h"
#include "Transform.h"

namespace Jam
{

Vec3 Transform::GetRight() const
{
    return Vec3::Transform(Numbers<Vec3>::k_right, m_rotation);
}

Vec3 Transform::GetLeft() const
{
    return Vec3::Transform(Numbers<Vec3>::k_left, m_rotation);
}

Vec3 Transform::GetUp() const
{
    return Vec3::Transform(Numbers<Vec3>::k_up, m_rotation);
}

Vec3 Transform::GetDown() const
{
    return Vec3::Transform(Numbers<Vec3>::k_down, m_rotation);
}

Vec3 Transform::GetForward() const
{
    return Vec3 ::Transform(Numbers<Vec3>::k_forward, m_rotation);
}

Vec3 Transform::GetBackward() const
{
    return Vec3 ::Transform(Numbers<Vec3>::k_backward, m_rotation);
}

const Mtx4& Transform::GetMatrix() const
{
    if (m_bDirty)
    {
        const Mtx4 s = Mtx4::CreateScale(m_scale);
        const Mtx4 r = Mtx4::CreateFromQuaternion(m_rotation);
        const Mtx4 t = Mtx4::CreateTranslation(m_translation);
        m_mtxCache   = s * r * t;
        m_bDirty     = false;
    }

    return m_mtxCache;
}

void Transform::SetMatrix(const Mtx4& _mtx)
{
    const Vec3 sr = { _mtx._11, _mtx._12, _mtx._13 };
    const Vec3 su = { _mtx._21, _mtx._22, _mtx._23 };
    const Vec3 sf = { _mtx._31, _mtx._32, _mtx._33 };

    const float rs = Max(Length(sr), Numbers<float>::k_epsilon);
    const float us = Max(Length(su), Numbers<float>::k_epsilon);
    const float fs = Max(Length(sf), Numbers<float>::k_epsilon);

    const Vec3 r = sr / rs;
    const Vec3 u = su / us;
    const Vec3 f = sf / fs;

    m_translation = Vec3 { _mtx._41, _mtx._42, _mtx._43 };
    m_scale       = Vec3 { rs, us, fs };
    m_rotation    = Quat::CreateFromRotationMatrix(Mtx4 { r, u, f });
    m_mtxCache    = _mtx;
    m_bDirty      = false;
}

Mtx4 Transform::CreateViewMatrix() const
{
    const Vec3 r = GetRight();
    const Vec3 u = GetUp();
    const Vec3 f = GetForward();

    const float tx = -Dot(r, m_translation);
    const float ty = -Dot(u, m_translation);
    const float tz = -Dot(f, m_translation);

    return Mtx4 {
        { r.x, u.x, f.x, 0.0f },
        { r.y, u.y, f.y, 0.0f },
        { r.z, u.z, f.z, 0.0f },
        {  tx,  ty,  tz, 1.0f }
    };
}

Transform Transform::Inverted() const
{
    Transform result = *this;
    result.Invert();
    return result;
}

void Transform::Invert()
{
    m_scale.x = IsZeroApprox(m_scale.x) ? 0.f : 1.f / m_scale.x;
    m_scale.y = IsZeroApprox(m_scale.y) ? 0.f : 1.f / m_scale.y;
    m_scale.z = IsZeroApprox(m_scale.z) ? 0.f : 1.f / m_scale.z;
    m_rotation.Conjugate();

    // S * R * T -> T' * R' * S'
    m_translation = Vec3::Transform(-m_translation, m_rotation) * m_scale;
    m_bDirty      = true;
}

Transform Transform::Multed(const Transform& _trans) const
{
    Transform result = *this;
    result.Mult(_trans);
    return result;
}

void Transform::Mult(const Transform& _trans)
{
    // *this * _trans = Child * Parnet
    // T 요소만 볼 때
    // T = Tc * Sp * Rp + Tp
    m_translation = Vec3::Transform(m_translation * _trans.m_scale, _trans.m_rotation) + _trans.m_translation;
    m_rotation    = m_rotation * _trans.m_rotation;
    m_scale       = m_scale * _trans.m_scale;
    m_bDirty      = true;
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
    m_scale  = _xyz;
    m_bDirty = true;
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
    m_bDirty  = true;
}
void Transform::SetScaleY(const float _y)
{
    m_scale.y = _y;
    m_bDirty  = true;
}
void Transform::SetScaleZ(const float _z)
{
    m_scale.z = _z;
    m_bDirty  = true;
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

    Vec3 axis;
    if (IsZeroApprox(s))
    {
        axis = Vec3 { 1.0f, 0.0f, 0.0f };
    }
    else
    {
        axis = Vec3 { m_rotation.x / s, m_rotation.y / s, m_rotation.z / s };
    }
    return { axis, rad };
}

Vec3 Transform::ToEuler() const
{
    const Vec3 r = GetRight();
    const Vec3 u = GetUp();
    const Vec3 f = GetForward();

    const float cy = Sqrt(f.z * f.z + f.x * f.x);
    const float cx = Atan2(-f.y, cy);

    if (cy > Numbers<float>::k_epsilon)
    {
        return { cx, Atan2(f.x, f.z), Atan2(r.y, u.y) };
    }
    else
    {
        return { cx, 0.f, Atan2(-u.x, r.x) };
    }
}

void Transform::SetRotation(const Quat _q)
{
    m_rotation = Normalized(_q);
    m_bDirty   = true;
}

void Transform::SetRotation(const Vec3 _axis, const float _rad)
{
    SetRotation(Quat::CreateFromAxisAngle(_axis, _rad));
}

void Transform::SetRotation(const Vec3 _eulerRad)
{
    SetRotation(Quat::CreateFromYawPitchRoll(_eulerRad.y, _eulerRad.x, _eulerRad.z));
}

void Transform::SetRotation(const float _pitchRad, const float _yawRad, const float _rollRad)
{
    SetRotation(Quat::CreateFromYawPitchRoll(_yawRad, _pitchRad, _rollRad));
}

void Transform::SetLookAt(const Vec3 _point)
{
    const Vec3 dir = Normalized(_point - m_translation);
    SetLookTo(dir);
}

void Transform::SetLookTo(const Vec3 _forward)
{
    const Vec3 f = Normalized(_forward);

    Vec3 base = Numbers<Vec3>::k_up;
    if (IsEqualApprox(Abs(Dot(base, f)), 1.f))
    {
        base = Numbers<Vec3>::k_right;
    }

    const Vec3 r = Normalized(Cross(base, f));
    const Vec3 u = Normalized(Cross(f, r));

    const Mtx4 mR {
        Vec4From(r, 0.f),
        Vec4From(u, 0.f),
        Vec4From(f, 0.f),
        { 0.f, 0.f, 0.f, 1.f }
    };

    m_rotation = Quat::CreateFromRotationMatrix(mR);
    m_bDirty   = true;
}

void Transform::AddRotation(const Quat _q)
{
    m_rotation = Normalized(m_rotation * _q);
    m_bDirty   = true;
}

void Transform::AddRotation(const Vec3 _axis, const float _rad)
{
    AddRotation(Quat::CreateFromAxisAngle(_axis, _rad));
}

void Transform::AddRotation(const Vec3 _eulerRad)
{
    AddRotation(Quat::CreateFromYawPitchRoll(_eulerRad.y, _eulerRad.x, _eulerRad.z));
}

void Transform::AddRotation(const float _pitchRad, const float _yawRad, const float _rollRad)
{
    AddRotation(Quat::CreateFromYawPitchRoll(_yawRad, _pitchRad, _rollRad));
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
    m_bDirty      = true;
}

void Transform::SetTranslation(float _x, float _y, float _z)
{
    SetTranslation({ _x, _y, _z });
}

void Transform::SetTranslationX(const float _x)
{
    m_translation.x = _x;
    m_bDirty        = true;
}
void Transform::SetTranslationY(const float _y)
{
    m_translation.y = _y;
    m_bDirty        = true;
}
void Transform::SetTranslationZ(const float _z)
{
    m_translation.z = _z;
    m_bDirty        = true;
}

void Transform::AddTranslation(const Vec3 _xyz)
{
    m_translation += _xyz;
    m_bDirty = true;
}

void Transform::AddTranslation(float _x, float _y, float _z)
{
    AddTranslation({ _x, _y, _z });
}

void Transform::AddTranslationX(const float _x)
{
    m_translation.x += _x;
    m_bDirty = true;
}
void Transform::AddTranslationY(const float _y)
{
    m_translation.y += _y;
    m_bDirty = true;
}
void Transform::AddTranslationZ(const float _z)
{
    m_translation.z += _z;
    m_bDirty = true;
}

bool Transform::IsDirty() const
{
    return m_bDirty;
}

}   // namespace Jam