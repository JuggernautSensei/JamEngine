#include "pch.h"

#include "MathUtilities.h"

namespace jam
{

namespace details
{
    std::mt19937 g_randomGenerator;
    UInt32       g_randomSeed;
}   // namespace details

PitchYaw ForwardToPitchYaw(const Vec3 _forward)
{
    const Vec3 f = Normalized(_forward);

    PitchYaw result;
    result.yaw   = ATan2(f.x, f.z);
    result.pitch = Asin(-f.y);
    return result;
}

Vec3 PitchYawToForward(const float _pitchRad, const float _yawRad)
{
    const auto [sp, cp] = SinCosApprox(_pitchRad);
    const auto [sy, cy] = SinCosApprox(_yawRad);
    return Normalized(Vec3 { sp * sy, -sp, cp * cy });
}

Float16 ToFloat16(const float value)
{
    return DirectX::PackedVector::XMConvertFloatToHalf(value);
}

float ToFloat32(const Float16 value)
{
    return DirectX::PackedVector::XMConvertHalfToFloat(value);
}

Mat4 CreateWorldLookTo(const Vec3 _t, const Vec3 _f, const Vec3 _s)
{
    const Vec3 f = Normalized(_f);

    Vec3 base = Vec3::Up;
    if (IsEqualApprox(Dot(base, f), 1.f))
    {
        base = Vec3::Forward;
    }

    const Vec3 r = Normalized(Cross(base, f));
    const Vec3 u = Normalized(Cross(f, r));

    const Mat4 rot {
        r.x, r.y, r.z, 0.f, u.x, u.y, u.z, 0.f, f.x, f.y, f.z, 0.f, 0.f, 0.f, 0.f, 1.f
    };

    return Mat4::CreateScale(_s) * rot * Mat4::CreateTranslation(_t);
}

Mat4 CreateWorld(const Vec3 _t, const Quat _q, const Vec3 _s)
{
    return Mat4::CreateScale(_s) * Mat4::CreateFromQuaternion(_q) * Mat4::CreateTranslation(_t);
}

Mat4 CreateViewLookTo(const Vec3 _position, const Vec3 _forward)
{
    const Vec3 f    = Normalized(_forward);
    Vec3       base = Vec3::Up;
    if (IsEqualApprox(Dot(base, f), 1.f))
    {
        base = Vec3::Forward;
    }

    return DirectX::XMMatrixLookToLH(_position, f, base);
}

Mat4 CreateViewLookAt(const Vec3 _position, const Vec3 _focusPoint)
{
    const Vec3 f = (_focusPoint - _position);
    return CreateViewLookTo(_position, f);
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

AxisAngle QuatToAxisAngle(const Quat _q)
{
    const Quat  q     = Normalized(_q);
    const Vec3  axis  = Normalized({ q.x, q.y, q.z });
    const float angle = 2.f * Acos(q.w);
    return { axis, angle };
}

}   // namespace jam