#include "pch.h"

#include "Math.h"

namespace Jam
{

namespace Details
{
    std::mt19937 g_randomGenerator = {};
    UInt32       g_randomSeed      = {};
}   // namespace Details

const Vec2i Vec2i::k_zero = { 0, 0 };
const Vec2i Vec2i::k_one  = { 1, 1 };

Float16 ToFloat16(
    const float value)
{
    return DirectX::PackedVector::XMConvertFloatToHalf(value);
}

float ToFloat32(
    const Float16 value)
{
    return DirectX::PackedVector::XMConvertHalfToFloat(value);
}

Vector3 EulerFrom(
    const Vector3 _forward)
{
    const Vector3  f  = Normalized(_forward);
    const float cy = Sqrt(f.z * f.z + f.x * f.x);
    const float cx = Atan2(-f.y, cy);
    if (cy > k_epsilon)
    {
        return { cx, Atan2(f.x, f.z), 0.0f };
    }

    return { cx, 0.0f, 0.0f };
}

Vector3 ForwardFrom(
    const Vector3 _eulerRad)
{
    const auto y = SinCosApprox(_eulerRad.y);
    const auto x = SinCosApprox(_eulerRad.x);
    return Normalized({ y.sin * x.cos, -x.sin, y.cos * x.cos });
}

Matrix4x4 CreateOrthoMatrix(
    const float width,
    const float height,
    const float nearZ,
    const float farZ)
{
    return DirectX::XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

Matrix4x4 CreateOrthoMatrix(
    const float left,
    const float right,
    const float bottom,
    const float top,
    const float nearZ,
    const float farZ)
{
    return DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);
}

Matrix4x4 CreatePerspectiveMatrix(
    const float fovYRad,
    const float aspect,
    const float zNear,
    const float zFar)
{
    return DirectX::XMMatrixPerspectiveFovLH(fovYRad, aspect, zNear, zFar);
}

}   // namespace Jam