#include "pch.h"

#include "MathUtilities.h"
#include "Transform.h"

namespace jam
{

Transform::Transform(const Mat4& _mtx)
    : m_mtx(_mtx)
{
}

const Mat4& Transform::GetMatrix() const
{
    return m_mtx;
}

Mat4 Transform::GetViewMatrix() const
{
    const Vec3 r = GetRight();
    const Vec3 u = GetUp();
    const Vec3 f = GetForward();
    const Vec3 t = GetTranslation();

    // reverse translation
    float tx = -Dot(t, r);
    float ty = -Dot(t, u);
    float tz = -Dot(t, f);

    return Mat4(
        { r.x, u.x, f.x, 0.0f },
        { r.y, u.y, f.y, 0.0f },
        { r.z, u.z, f.z, 0.0f },
        { tx, ty, tz, 1.0f });
}

Mat4 Transform::GetInvertMatrix() const
{
    const Vec3 r = GetRight();
    const Vec3 u = GetUp();
    const Vec3 f = GetForward();
    const Vec3 t = GetTranslation();

    // reverse translation
    float tx = -Dot(t, r);
    float ty = -Dot(t, u);
    float tz = -Dot(t, f);

    // reverse scale
    const Vec3 scale = GetScale();
    float      sx    = 1.0f / scale.x;
    float      sy    = 1.0f / scale.y;
    float      sz    = 1.0f / scale.z;

    return Mat4(
        { r.x * sx, u.x * sy, f.x * sz, 0.0f },
        { r.y * sx, u.y * sy, f.y * sz, 0.0f },
        { r.z * sx, u.z * sy, f.z * sz, 0.0f },
        { tx, ty, tz, 1.0f });
}

Transform Transform::GetInvert() const
{
    Transform inv;
    inv.SetMatrix(GetInvertMatrix());
    return inv;
}

Vec3 Transform::GetRight() const
{
    return Normalized(m_mtx.Right());
}

Vec3 Transform::GetLeft() const
{
    return -GetRight();
}

Vec3 Transform::GetUp() const
{
    return Normalized(m_mtx.Up());
}

Vec3 Transform::GetDown() const
{
    return -GetUp();
}

Vec3 Transform::GetForward() const
{
    return Normalized(m_mtx.Forward());
}

Vec3 Transform::GetBackward() const
{
    return -GetForward();
}

Mat4 Transform::GetRotationMatrix() const
{
    return Mat4 { GetRight(), GetUp(), GetForward() };
}

Mat4 Transform::GetScaleMatrix() const
{
    const Vec3 scale = GetScale();
    return {
        scale.x, 0.0f, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, 0.0f, scale.z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
    };
}

Mat4 Transform::GetTranslationMatrix() const
{
    return {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, m_mtx._41, m_mtx._42, m_mtx._43, 1.0f
    };
}

Vec3 Transform::GetTranslation() const
{
    return m_mtx.Translation();
}

float Transform::GetTranslationX() const
{
    return m_mtx.Translation().x;
}

float Transform::GetTranslationY() const
{
    return m_mtx.Translation().y;
}

float Transform::GetTranslationZ() const
{
    return m_mtx.Translation().z;
}

Vec3 Transform::GetScale() const
{
    const float sx = Length(m_mtx.Right());
    const float sy = Length(m_mtx.Up());
    const float sz = Length(m_mtx.Forward());
    return { sx, sy, sz };
}

float Transform::GetScaleX() const
{
    return GetScale().x;
}

float Transform::GetScaleY() const
{
    return GetScale().y;
}

float Transform::GetScaleZ() const
{
    return GetScale().z;
}

Quat Transform::GetQuaternion() const
{
    return Quat::CreateFromRotationMatrix(GetRotationMatrix());
}

AxisAngle Transform::GetAxisAngle() const
{
    return QuatToAxisAngle(GetQuaternion());
}

Vec3 Transform::GetPitchYawRoll() const
{
    return GetRotationMatrix().ToEuler();
}

Transform& Transform::MultMatrix(const Mat4& _mtx)
{
    m_mtx *= _mtx;
    return *this;
}

Transform& Transform::SetMatrix(const Mat4& _mtx)
{
    m_mtx = _mtx;
    return *this;
}

Transform& Transform::SetTranslation(const Vec3 _translation)
{
    m_mtx._41 = _translation.x;
    m_mtx._42 = _translation.y;
    m_mtx._43 = _translation.z;
    return *this;
}

Transform& Transform::SetTranslation(const float _x, const float _y, const float _z)
{
    return SetTranslation(Vec3 { _x, _y, _z });
}
Transform& Transform::SetTranslationX(const float _x)
{
    m_mtx._41 = _x;
    return *this;
}

Transform& Transform::SetTranslationY(const float _y)
{
    m_mtx._42 = _y;
    return *this;
}

Transform& Transform::SetTranslationZ(const float _z)
{
    m_mtx._43 = _z;
    return *this;
}

Transform& Transform::AddTranslation(const Vec3 _translation)
{
    m_mtx._41 += _translation.x;
    m_mtx._42 += _translation.y;
    m_mtx._43 += _translation.z;
    return *this;
}

Transform& Transform::AddTranslation(const float _x, const float _y, const float _z)
{
    m_mtx._41 += _x;
    m_mtx._42 += _y;
    m_mtx._43 += _z;
    return *this;
}

Transform& Transform::AddTranslationX(const float _x)
{
    m_mtx._41 += _x;
    return *this;
}

Transform& Transform::AddTranslationY(const float _y)
{
    m_mtx._42 += _y;
    return *this;
}

Transform& Transform::AddTranslationZ(const float _z)
{
    m_mtx._43 += _z;
    return *this;
}

Transform& Transform::SetRotation(const Quat _rotation)
{
    const Vec3 t = GetTranslation();
    const Vec3 s = GetScale();
    m_mtx        = Mat4::CreateScale(s) * Mat4::CreateFromQuaternion(_rotation);
    m_mtx._41    = t.x;
    m_mtx._42    = t.y;
    m_mtx._43    = t.z;
    return *this;
}

Transform& Transform::SetRotation(const Vec3 _axis, const float _angleRad)
{
    return SetRotation(Quat::CreateFromAxisAngle(_axis, _angleRad));
}

Transform& Transform::SetRotation(const Vec3 _pitchYawRollRad)
{
    return SetRotation(Quat::CreateFromYawPitchRoll(_pitchYawRollRad.y, _pitchYawRollRad.x, _pitchYawRollRad.z));
}

Transform& Transform::SetRotation(const float _pitchRad, const float _yawRad, const float _rollRad)
{
    return SetRotation(Quat::CreateFromYawPitchRoll(_yawRad, _pitchRad, _rollRad));
}

Transform& Transform::SetLookAt(const Vec3 _point)
{
    const Vec3 t       = GetTranslation();
    const Vec3 forward = _point - t;
    return SetLookTo(forward);
}

Transform& Transform::SetLookTo(const Vec3 _forward)
{
    const Vec3 t = GetTranslation();
    const Vec3 s = GetScale();
    const Vec3 f = Normalized(_forward);
    m_mtx        = CreateWorldLookTo(t, f, s);
    return *this;
}

Transform& Transform::AddRotation(const Quat _q)
{
    const Quat q = GetQuaternion();
    return SetRotation(q * _q);
}

Transform& Transform::AddRotation(const Vec3 _axis, const float _angleRad)
{
    return AddRotation(Quat::CreateFromAxisAngle(_axis, _angleRad));
}

Transform& Transform::SetScale(const Vec3 _scale)
{
    if (IsZeroApprox(_scale.x) || IsZeroApprox(_scale.y) || IsZeroApprox(_scale.z))
        return *this;

    const Vec3 r = GetRight();
    const Vec3 u = GetUp();
    const Vec3 f = GetForward();

    m_mtx.Right(r * _scale.x);
    m_mtx.Up(u * _scale.y);
    m_mtx.Forward(f * _scale.z);

    return *this;
}

Transform& Transform::SetScale(const float _x, const float _y, const float _z)
{
    return SetScale(Vec3 { _x, _y, _z });
}

Transform& Transform::SetScale(const float _uniform)
{
    return SetScale(Vec3 { _uniform, _uniform, _uniform });
}

Transform& Transform::SetScaleX(const float _x)
{
    Vec3 s = GetScale();
    s.x    = _x;
    return SetScale(s);
}

Transform& Transform::SetScaleY(const float _y)
{
    Vec3 s = GetScale();
    s.y    = _y;
    return SetScale(s);
}

Transform& Transform::SetScaleZ(const float _z)
{
    Vec3 s = GetScale();
    s.z    = _z;
    return SetScale(s);
}

Transform& Transform::AddScale(const Vec3 _s)
{
    Vec3 s = GetScale();
    return SetScale(s + _s);
}

Transform& Transform::AddScale(const float _x, const float _y, const float _z)
{
    return AddScale({ _x, _y, _z });
}

Transform& Transform::AddScale(const float _s)
{
    return AddScale({ _s, _s, _s });
}

Transform& Transform::AddScaleX(const float _x)
{
    Vec3 s = GetScale();
    s.x += _x;
    return SetScale(s);
}

Transform& Transform::AddScaleY(const float _y)
{
    Vec3 s = GetScale();
    s.y += _y;
    return SetScale(s);
}

Transform& Transform::AddScaleZ(const float _z)
{
    Vec3 s = GetScale();
    s.z += _z;
    return SetScale(s);
}

}   // namespace jam