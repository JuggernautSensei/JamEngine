#include "pch.h"

#include "Math.h"

namespace jam
{

namespace details
{
    std::mt19937 g_randomGenerator;
    UInt32       g_randomSeed;
}   // namespace details

Float16 ToFloat16(const float value)
{
    return DirectX::PackedVector::XMConvertFloatToHalf(value);
}

float ToFloat32(const Float16 value)
{
    return DirectX::PackedVector::XMConvertHalfToFloat(value);
}

Vec2 ForwardToPitchYaw(const Vec3 _forward)
{
    const Vec3 f = Normalized(_forward);

    Vec2 py;
    py.x = Asin(-f.y);        // pitch
    py.y = Atan2(f.x, f.z);   // yaw
    return py;
}

Vec3 PitchYawToForward(const float pitch, const float yaw)
{
    const Quat q = Quat::CreateFromYawPitchRoll(yaw, pitch, 0.f);
    return Vec3::Transform(Vec3::Forward, q);
}

Mat4 CreateOrthoMatrix(const float width, const float height, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

Mat4 CreateOrthoMatrix(const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);
}

Mat4 CreatePerspectiveMatrix(const float fovYRad, const float aspect, const float zNear, const float zFar)
{
    return DirectX::XMMatrixPerspectiveFovLH(fovYRad, aspect, zNear, zFar);
}

}   // namespace jam