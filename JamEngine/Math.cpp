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

Vec3 EulerFrom(
    const Vec3 _forward)
{
    const Vec3  f  = Normalized(_forward);
    const float cy = Sqrt(f.z * f.z + f.x * f.x);
    const float cx = Atan2(-f.y, cy);
    if (cy > Numbers<float>::k_epsilon)
    {
        return { cx, Atan2(f.x, f.z), 0.0f };
    }

    return { cx, 0.0f, 0.0f };
}

Vec3 ForwardFrom(
    const Vec3 _eulerRad)
{
    const float sy = Sin(_eulerRad.y);
    const float cy = Cos(_eulerRad.y);
    const float sx = Sin(_eulerRad.x);
    const float cx = Cos(_eulerRad.x);
    return Normalized({ sy * cx, -sx, cy * cx });
}

Mtx4 CreateOrthoMatrix(
    const float width,
    const float height,
    const float nearZ,
    const float farZ)
{
    return DirectX::XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

Mtx4 CreateOrthoMatrix(
    const float left,
    const float right,
    const float bottom,
    const float top,
    const float nearZ,
    const float farZ)
{
    return DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);
}

Mtx4 CreatePerspectiveMatrix(
    const float fovYRad,
    const float aspect,
    const float zNear,
    const float zFar)
{
    return DirectX::XMMatrixPerspectiveFovLH(fovYRad, aspect, zNear, zFar);
}

}   // namespace Jam