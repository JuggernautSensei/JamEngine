#include "pch.h"

#include "MathUtilities.h"
#include <Random>

namespace jam::details
{
std::mt19937 g_randomGenerator = std::mt19937 {};
UInt32       g_randomSeed      = 0;
}   // namespace jam::details

namespace jam
{

Vec3 ToRad(const Vec3& _degrees)
{
    return _degrees * k_degToRad;
}

Vec3 ToDeg(const Vec3& _radians)
{
    return _radians * k_radToDeg;
}

Float16 ToFloat16(const float value)
{
    return DirectX::PackedVector::XMConvertFloatToHalf(value);
}

float ToFloat(const Float16 value)
{
    return DirectX::PackedVector::XMConvertHalfToFloat(value);
}

Mat4 CreateWorldLookTo(const Vec3& _translate, const Vec3& _forward, const Vec3& _scale)
{
    // forward
    Vec3 forward = _forward;
    forward.Normalize();

    Vec3 up = Vec3::Up;
    if (up.Dot(forward) >= 1.f - k_epsilon)
        up = Vec3::Forward;

    // right
    Vec3 right = up.Cross(forward);
    right.Normalize();

    // up
    up = forward.Cross(right);
    up.Normalize();

    // clang-format off
    Mat4 rMat {
        right.x, right.y, right.z, 0.f,
        up.x, up.y, up.z, 0.f,
        forward.x, forward.y, forward.z, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    // clang-format on

    return Mat4::CreateScale(_scale)
         * rMat
         * Mat4::CreateTranslation(_translate);
}

Mat4 CreateWorld(const Vec3& _translate, const Quat& _quat, const Vec3& _scale)
{
    return Mat4::CreateScale(_scale)
         * Mat4::CreateFromQuaternion(_quat)
         * Mat4::CreateTranslation(_translate);
}

Mat4 CreateViewLookTo(const Vec3& _position, const Vec3& _forward)
{
    Vec3 forward = _forward;
    forward.Normalize();
    Vec3 up = Vec3::Up;
    if (up.Dot(forward) >= 1.f - k_epsilon)
        up = Vec3::Forward;
    return DirectX::XMMatrixLookToLH(_position, _forward, Vec3::Up);
}

Mat4 CreateViewLookAt(const Vec3& _position, const Vec3& _focusPoint)
{
    const Vec3 forward = (_focusPoint - _position);
    return CreateViewLookTo(_position, forward);
}

Mat4 CreateOrthographic(const float width, const float height, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

Mat4 CreateOrthographic(const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);
}

Mat4 CreatePerspectiveFov(const float fovYRad, const float aspectRatio, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixPerspectiveFovLH(fovYRad, aspectRatio, nearZ, farZ);
}

AxisAngle QuatToAxisAngle(const Quat& _q)
{
    Quat q = _q;
    q.Normalize();

    Vec3 axis = { q.x, q.y, q.z };
    axis.Normalize();

    float angle = 2.f * std::acosf(q.w);
    return { axis, angle };
}

}   // namespace jam