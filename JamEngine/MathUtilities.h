#pragma once

namespace jam
{

constexpr float k_pi       = 3.14159265358979323846f;   // pi constant
constexpr float k_2pi      = 2.f * k_pi;                // 2 * pi constant
constexpr float k_piDiv2   = k_pi / 2.f;                // pi / 2 constant
constexpr float k_piDiv4   = k_pi / 4.f;                // pi / 4 constant
constexpr float k_1DivPi   = 1.f / k_pi;                // 1 / pi constant
constexpr float k_1Div2Pi  = 1.f / (2.f * k_pi);        // 1 / (2 * pi) constant
constexpr float k_epsilon  = 1e-6f;                     // epsilon constant for floating point comparison
constexpr float k_radToDeg = 180.f / k_pi;              // radian to degree conversion factor
constexpr float k_degToRad = k_pi / 180.f;              // degree to radian conversion factor

constexpr NODISCARD float ToRadians(const float degrees) { return degrees * k_degToRad; }
constexpr NODISCARD float ToDegrees(const float radians) { return radians * k_radToDeg; }
constexpr NODISCARD float Absolute(const float value) { return value < 0.f ? -value : value; }
constexpr NODISCARD bool  IsNearlyEqual(const float a, const float b) { return Absolute(a - b) < k_epsilon; }
constexpr NODISCARD bool  IsNearlyZero(const float value) { return Absolute(value) < k_epsilon; }

constexpr NODISCARD std::pair<float, float> ComputeSinCos(const float _radian)
{
    float quotient = k_1Div2Pi * _radian;
    if (_radian >= 0.0f)
    {
        quotient = static_cast<float>(static_cast<int>(quotient + 0.5f));
    }
    else
    {
        quotient = static_cast<float>(static_cast<int>(quotient - 0.5f));
    }
    float y = _radian - k_2pi * quotient;

    float sign;
    if (y > k_piDiv2)
    {
        y    = k_pi - y;
        sign = -1.0f;
    }
    else if (y < -k_piDiv2)
    {
        y    = -k_pi - y;
        sign = -1.0f;
    }
    else
    {
        sign = +1.0f;
    }

    const float y2 = y * y;

    return {
        (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y,
        sign * ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f
    };
}

constexpr NODISCARD Vec2 ToVec2(const Vec3& vec3) { return { vec3.x, vec3.y }; }
constexpr NODISCARD Vec2 ToVec2(const Vec4& vec4) { return { vec4.x, vec4.y }; }
constexpr NODISCARD Vec3 ToVec3(const Vec2& vec2, const float z = 0.f) { return { vec2.x, vec2.y, z }; }
constexpr NODISCARD Vec3 ToVec3(const Vec4& vec4) { return { vec4.x, vec4.y, vec4.z }; }
constexpr NODISCARD Vec4 ToVec4(const Vec2& vec2, const float z = 0.f, const float w = 1.f) { return { vec2.x, vec2.y, z, w }; }
constexpr NODISCARD Vec4 ToVec4(const Vec3& vec3, const float w = 1.f) { return { vec3.x, vec3.y, vec3.z, w }; }

inline NODISCARD Vec2 Normalize(const Vec2& vec)
{
    Vec2 v = vec;
    v.Normalize();
    return v;
}

inline NODISCARD Vec3 Normalize(const Vec3& vec)
{
    Vec3 v = vec;
    v.Normalize();
    return v;
}

inline NODISCARD Vec4 Normalize(const Vec4& vec)
{
    Vec4 v = vec;
    v.Normalize();
    return v;
}

inline NODISCARD Quat Normalize(const Quat& quat)
{
    Quat q = quat;
    q.Normalize();
    return q;
}

// matrix
inline NODISCARD Mat4 CreateWorldMatrix(const Vec3& position, const Quat& rotation, const Vec3& scale)
{
    return Mat4::CreateScale(scale) * Mat4::CreateFromQuaternion(rotation) * Mat4::CreateTranslation(position);
}

NODISCARD Mat4        CreateViewMatrixFromDirection(const Vec3& position, const Vec3& lookDir);
NODISCARD inline Mat4 CreateViewMatrixFromPoint(const Vec3& position, const Vec3& focusPoint)
{
    const Vec3 lookDir = (focusPoint - position);
    return CreateViewMatrixFromDirection(position, lookDir);
}

NODISCARD inline Mat4 CreateOrthographicMatrix(const float width, const float height, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixOrthographicLH(width, height, nearZ, farZ);
}

NODISCARD inline Mat4 CreateOrthographicMatrix(const float aspectRatio, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixOrthographicLH(aspectRatio, 1.f, nearZ, farZ);
}

NODISCARD inline Mat4 CreateOrthographicMatrix(const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);
}

NODISCARD inline Mat4 CreatePerspectiveMatrix(const float fovYRad, const float aspectRatio, const float nearZ, const float farZ)
{
    return DirectX::XMMatrixPerspectiveFovLH(fovYRad, aspectRatio, nearZ, farZ);
}

}   // namespace jam