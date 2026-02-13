#include "pch.h"

#include "Math.h"
#include "Transform.h"

namespace Jam
{

Vec3 Transform::GetRight() const
{
    return Normalized(m_right);
}

Vec3 Transform::GetLeft() const
{
    return -Normalized(m_right);
}

Vec3 Transform::GetUp() const
{
    return Normalized(m_up);
}

Vec3 Transform::GetDown() const
{
    return -Normalized(m_up);
}

Vec3 Transform::GetForward() const
{
    return Normalized(m_forward);
}

Vec3 Transform::GetBackward() const
{
    return -Normalized(m_forward);
}

const Mtx4& Transform::GetMatrix() const
{
    // update dirty
    if (m_bDirty)
    {
        m_mtxCache = Mtx4 {
            Vec4From(m_right, 0.0f),
            Vec4From(m_up, 0.0f),
            Vec4From(m_forward, 0.0f),
            Vec4From(m_origin, 1.0f)
        };

        m_bDirty = false;
    }

    return m_mtxCache;
}

void Transform::SetMatrix(
    const Mtx4& _mtx)
{
    m_right    = _mtx.Right();
    m_up       = _mtx.Up();
    m_forward  = _mtx.Forward();
    m_origin   = _mtx.Translation();
    m_bDirty   = false;
    m_mtxCache = _mtx;
}

Mtx4 Transform::ToViewMatrix() const
{
    const Vec3 r = Normalized(m_right);
    const Vec3 u = Normalized(m_up);
    const Vec3 f = Normalized(m_forward);

    const Vec3  t  = m_origin;
    const float tx = -Dot(r, t);
    const float ty = -Dot(u, t);
    const float tz = -Dot(f, t);

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
    const float invSx = InvLength(m_right);
    const float invSy = InvLength(m_up);
    const float invSz = InvLength(m_forward);

    const Vec3 r = m_right * invSx;
    const Vec3 u = m_up * invSy;
    const Vec3 f = m_forward * invSz;

    const float tx = -Dot(r, m_origin);
    const float ty = -Dot(u, m_origin);
    const float tz = -Dot(f, m_origin);

    m_right   = Vec3 { r.x * invSx, u.x * invSy, f.x * invSz };
    m_up      = Vec3 { r.y * invSx, u.y * invSy, f.y * invSz };
    m_forward = Vec3 { r.z * invSx, u.z * invSy, f.z * invSz };
    m_origin  = Vec3 { tx, ty, tz };
    m_bDirty  = true;
}

Transform Transform::Multed(
    const Transform& _trans) const
{
    Transform result = *this;
    result.Mult(_trans);
    return result;
}

void Transform::Mult(
    const Transform& _trans)
{
    SetMatrix(GetMatrix() * _trans.GetMatrix());
}

Vec3 Transform::GetScale() const
{
    return { Length(m_right), Length(m_up), Length(m_forward) };
}

float Transform::GetScaleX() const
{
    return Length(m_right);
}

float Transform::GetScaleY() const
{
    return Length(m_up);
}

float Transform::GetScaleZ() const
{
    return Length(m_forward);
}

void Transform::SetScale(const Vec3 _xyz)
{
    const Vec3 r = Normalized(m_right);
    const Vec3 u = Normalized(m_up);
    const Vec3 f = Normalized(m_forward);

    m_right   = r * _xyz.x;
    m_up      = u * _xyz.y;
    m_forward = f * _xyz.z;
    m_bDirty  = true;
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
    const Vec3 r = Normalized(m_right);
    m_right      = r * _x;
    m_bDirty     = true;
}

void Transform::SetScaleY(const float _y)
{
    const Vec3 u = Normalized(m_up);
    m_up         = u * _y;
    m_bDirty     = true;
}

void Transform::SetScaleZ(const float _z)
{
    const Vec3 f = Normalized(m_forward);
    m_forward    = f * _z;
    m_bDirty     = true;
}

void Transform::AddScale(const Vec3 _xyz)
{
    SetScale(GetScale() + _xyz);
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
    SetScaleX(GetScaleX() + _x);
}

void Transform::AddScaleY(const float _y)
{
    SetScaleY(GetScaleY() + _y);
}

void Transform::AddScaleZ(const float _z)
{
    SetScaleZ(GetScaleZ() + _z);
}

Quat Transform::ToQuat() const
{
    const Vec3 r   = Normalized(m_right);
    const Vec3 u   = Normalized(m_up);
    const Vec3 f   = Normalized(m_forward);
    const Mtx4 mtx = Mtx4 { r, u, f };
    return Quat::CreateFromRotationMatrix(mtx);
}

AxisAngle Transform::ToAxisAngle() const
{
    const Quat  q    = ToQuat();
    const float rad  = 2.f * Acos(q.w);
    const float sqrt = Sqrt(1.f - q.w * q.w);

    Vec3 axis = {};
    if (IsZeroApprox(sqrt))
    {
        axis = Vec3 { 1.0f, 0.0f, 0.0f };
    }
    else
    {
        axis = Vec3 { q.x / sqrt, q.y / sqrt, q.z / sqrt };
    }
    return { axis, rad };
}

Vec3 Transform::ToEuler() const
{
    const Vec3 r = Normalized(m_right);
    const Vec3 u = Normalized(m_up);
    const Vec3 f = Normalized(m_forward);

    const float cy = Sqrt(f.z * f.z + f.x * f.x);
    const float cx = Atan2(-f.y, cy);
    if (cy > k_epsilon)
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
    const Vec3 s   = GetScale();
    const Mtx4 mtx = Mtx4::CreateFromQuaternion(_q);

    m_right   = mtx.Right() * s.x;
    m_up      = mtx.Up() * s.y;
    m_forward = mtx.Forward() * s.z;
    m_bDirty  = true;
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
    const Vec3 dir = Normalized(_point - m_origin);
    SetLookTo(dir);
}

void Transform::SetLookTo(const Vec3 _forward)
{
    const Vec3 s = GetScale();
    const Vec3 f = Normalized(_forward);

    Vec3 base = vec3::k_up;
    if (IsEqualApprox(Abs(Dot(base, f)), 1.f))
    {
        base = vec3::k_right;
    }

    const Vec3 r = Normalized(Cross(base, f));
    const Vec3 u = Normalized(Cross(f, r));

    m_right   = r * s.x;
    m_up      = u * s.y;
    m_forward = f * s.z;
    m_bDirty  = true;
}

void Transform::AddRotation(const Quat _q)
{
    const Quat q = ToQuat();
    SetRotation(q * _q);
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
    AddRotation(_rad, 0.0f, 0.0f);
}

void Transform::AddRotationYaw(const float _rad)
{
    AddRotation(0.0f, _rad, 0.0f);
}

void Transform::AddRotationRoll(const float _rad)
{
    AddRotation(0.0f, 0.0f, _rad);
}

Vec3 Transform::GetTranslation() const
{
    return m_origin;
}

float Transform::GetTranslationX() const
{
    return m_origin.x;
}

float Transform::GetTranslationY() const
{
    return m_origin.y;
}

float Transform::GetTranslationZ() const
{
    return m_origin.z;
}

void Transform::SetTranslation(const Vec3 _xyz)
{
    m_origin = _xyz;
    m_bDirty = true;
}

void Transform::SetTranslation(float _x, float _y, float _z)
{
    SetTranslation({ _x, _y, _z });
}

void Transform::SetTranslationX(const float _x)
{
    m_origin.x = _x;
    m_bDirty   = true;
}

void Transform::SetTranslationY(const float _y)
{
    m_origin.y = _y;
    m_bDirty   = true;
}

void Transform::SetTranslationZ(const float _z)
{
    m_origin.z = _z;
    m_bDirty   = true;
}

void Transform::AddTranslation(const Vec3 _xyz)
{
    m_origin += _xyz;
    m_bDirty = true;
}

void Transform::AddTranslation(float _x, float _y, float _z)
{
    AddTranslation({ _x, _y, _z });
}

void Transform::AddTranslationX(const float _x)
{
    m_origin.x += _x;
    m_bDirty = true;
}

void Transform::AddTranslationY(const float _y)
{
    m_origin.y += _y;
    m_bDirty = true;
}

void Transform::AddTranslationZ(const float _z)
{
    m_origin.z += _z;
    m_bDirty = true;
}

}   // namespace Jam
