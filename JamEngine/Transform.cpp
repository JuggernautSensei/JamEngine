#include "pch.h"

#include "MathUtilities.h"
#include "Transform.h"

namespace jam
{

const Mat4& Transform::GetMatrix() const
{
    return m_mtx;
}

Mat4 Transform::GetViewMatrix() const
{
    Vec3 right   = { m_mtx._11, m_mtx._12, m_mtx._13 };
    Vec3 up      = { m_mtx._21, m_mtx._22, m_mtx._23 };
    Vec3 forward = { m_mtx._31, m_mtx._32, m_mtx._33 };
    Vec3 trans   = { m_mtx._41, m_mtx._42, m_mtx._43 };

    float tx = -trans.Dot(right);
    float ty = -trans.Dot(up);
    float tz = -trans.Dot(forward);

    return Mat4(
        { right.x, up.x, forward.x, 0.0f },
        { right.y, up.y, forward.y, 0.0f },
        { right.z, up.z, forward.z, 0.0f },
        { tx, ty, tz, 1.0f });
}
Vec3 Transform::GetRight() const
{
    return m_mtx.Right();
}

Vec3 Transform::GetLeft() const
{
    return m_mtx.Left();
}

Vec3 Transform::GetUp() const
{
    return m_mtx.Up();
}

Vec3 Transform::GetDown() const
{
    return m_mtx.Down();
}

Vec3 Transform::GetForward() const
{
    return m_mtx.Forward();
}

Vec3 Transform::GetBackward() const
{
    return m_mtx.Backward();
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
    using namespace DirectX;
    float sx = XMVectorGetX(XMVector3Length(m_mtx.Right()));
    float sy = XMVectorGetX(XMVector3Length(m_mtx.Up()));
    float sz = XMVectorGetX(XMVector3Length(m_mtx.Forward()));
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
    Vec3 scale = GetScale();
    Mat4 rMat {
        GetRight() / Max(Absolute(scale.x), k_epsilon),
        GetUp() / Max(Absolute(scale.y), k_epsilon),
        GetForward() / Max(Absolute(scale.z), k_epsilon)
    };
    return Quat::CreateFromRotationMatrix(rMat);
}

AxisAngle Transform::GetAxisAngle() const
{
    return QuatToAxisAngle(GetQuaternion());
}

Vec3 Transform::GetPitchYawRoll() const
{
    return GetQuaternion().ToEuler();
}

Transform& Transform::Multiply(const Mat4& _mat)
{
    m_mtx *= _mat;
    return *this;
}

Transform Transform::Multiplied(const Mat4& _mat) const
{
    Transform result = *this;
    result.Multiply(_mat);
    return result;
}

Transform& Transform::SetMatrix(const Mat4& _mat)
{
    m_mtx = _mat;
    return *this;
}

Transform& Transform::SetTranslation(const Vec3& _translation)
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

Transform& Transform::AddTranslation(const Vec3& _translation)
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

Transform& Transform::SetRotation(const Quat& _rotation)
{
    Vec3 translation = GetTranslation();
    Vec3 scale       = GetScale();
    m_mtx            = Mat4::CreateScale(scale) * Mat4::CreateFromQuaternion(_rotation);
    m_mtx._41        = translation.x;
    m_mtx._42        = translation.y;
    m_mtx._43        = translation.z;
    return *this;
}

Transform& Transform::SetRotation(const Vec3& _axis, const float _angleRad)
{
    return SetRotation(Quat::CreateFromAxisAngle(_axis, _angleRad));
}

Transform& Transform::SetRotation(const Vec3& _pitchYawRollRad)
{
    return SetRotation(Quat::CreateFromYawPitchRoll(_pitchYawRollRad.y, _pitchYawRollRad.x, _pitchYawRollRad.z));
}

Transform& Transform::SetRotation(const float _pitchRad, const float _yawRad, const float _rollRad)
{
    return SetRotation(Quat::CreateFromYawPitchRoll(_yawRad, _pitchRad, _rollRad));
}

Transform& Transform::SetLookAt(const Vec3& _point)
{
    Vec3 translation = GetTranslation();
    Vec3 lookDir     = _point - translation;
    return SetLookTo(lookDir);
}

Transform& Transform::SetLookTo(const Vec3& _lookDir)
{
    Vec3 translation = GetTranslation();
    Vec3 scale       = GetScale();

    Vec3 forward = _lookDir;
    forward.Normalize();

    Vec3 up = Vec3::Up;
    if (Absolute(forward.Dot(up)) > 1.f - k_epsilon)
        up = Vec3::Forward;

    Vec3 right = up.Cross(forward);
    right.Normalize();

    up = forward.Cross(right);
    up.Normalize();

    m_mtx     = Mat4(right * scale.x, up * scale.y, forward * scale.z);
    m_mtx._41 = translation.x;
    m_mtx._42 = translation.y;
    m_mtx._43 = translation.z;

    return *this;
}

Transform& Transform::AddRotation(const Quat& _rotation)
{
    Quat curRotation = GetQuaternion();
    return SetRotation(curRotation * _rotation);
}

Transform& Transform::AddRotation(const Vec3& _axis, const float _angleRad)
{
    return AddRotation(Quat::CreateFromAxisAngle(_axis, _angleRad));
}

Transform& Transform::SetScale(const Vec3& _scale)
{
    Vec3 curScale = GetScale();

    Vec3 right   = GetRight();
    Vec3 up      = GetUp();
    Vec3 forward = GetForward();

    m_mtx.Right((right * _scale.x) / Max(Absolute(curScale.x), k_epsilon));
    m_mtx.Up((up * _scale.y) / Max(Absolute(curScale.y), k_epsilon));
    m_mtx.Forward((forward * _scale.z) / Max(Absolute(curScale.z), k_epsilon));

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
    Vec3 scale = GetScale();
    scale.x    = _x;
    return SetScale(scale);
}

Transform& Transform::SetScaleY(const float _y)
{
    Vec3 scale = GetScale();
    scale.y    = _y;
    return SetScale(scale);
}

Transform& Transform::SetScaleZ(const float _z)
{
    Vec3 scale = GetScale();
    scale.z    = _z;
    return SetScale(scale);
}

Transform& Transform::AddScale(const Vec3& _scale)
{
    Vec3 curScale = GetScale();
    return SetScale(curScale + _scale);
}

Transform& Transform::AddScale(const float _x, const float _y, const float _z)
{
    return AddScale(Vec3 { _x, _y, _z });
}

Transform& Transform::AddScale(const float _uniform)
{
    return AddScale(Vec3 { _uniform, _uniform, _uniform });
}

Transform& Transform::AddScaleX(const float _x)
{
    Vec3 scale = GetScale();
    scale.x += _x;
    return SetScale(scale);
}

Transform& Transform::AddScaleY(const float _y)
{
    Vec3 scale = GetScale();
    scale.y += _y;
    return SetScale(scale);
}

Transform& Transform::AddScaleZ(const float _z)
{
    Vec3 scale = GetScale();
    scale.z += _z;
    return SetScale(scale);
}

}   // namespace jam