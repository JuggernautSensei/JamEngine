#pragma once

namespace jam
{

constexpr float k_pi       = 3.14159265358979323846f;   // pi constant
constexpr float k_2pi      = 2.f * k_pi;                // 2 * pi constant
constexpr float k_piDiv2   = k_pi / 2.f;                // pi / 2 constant
constexpr float k_piDiv4   = k_pi / 4.f;                // pi / 4 constant
constexpr float k_epsilon  = 1e-6f;                     // epsilon constant for floating point comparison
constexpr float k_radToDeg = 180.f / k_pi;              // radian to degree conversion factor
constexpr float k_degToRad = k_pi / 180.f;              // degree to radian conversion factor

NODISCARD constexpr float ToRadians(const float degrees)
{
    return degrees * k_degToRad;
}

NODISCARD constexpr float ToDegrees(const float radians)
{
    return radians * k_radToDeg;
}

NODISCARD constexpr float Absolute(const float value)
{
    return value < 0.f ? -value : value;
}

NODISCARD constexpr bool IsNearlyEqual(const float a, const float b)
{
    return Absolute(a - b) < k_epsilon;
}

NODISCARD constexpr bool IsNearlyZero(const float value)
{
    return Absolute(value) < k_epsilon;
}

}   // namespace jam