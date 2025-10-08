#include "Test.h"
#include <cmath>
#include <limits>

float Vec3::length() const noexcept
{
    return std::sqrt(length_sq());
}

float Vec3::length_sq() const noexcept
{
    return x * x + y * y + z * z;
}

Vec3 Vec3::normalized() const noexcept
{
    const float lsq = length_sq();
    if (lsq <= std::numeric_limits<float>::min())
    {
        return { 0.f, 0.f, 0.f };
    }
    const float inv = 1.0f / std::sqrt(lsq);
    return { x * inv, y * inv, z * inv };
}

void Vec3::normalize() noexcept
{
    const float lsq = length_sq();
    if (lsq <= std::numeric_limits<float>::min())
    {
        x = y = z = 0.f;
        return;
    }
    const float inv = 1.0f / std::sqrt(lsq);
    x *= inv;
    y *= inv;
    z *= inv;
}

bool Vec3::is_finite() const noexcept
{
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

bool Vec3::approx_equal(const Vec3& rhs, float eps) const noexcept
{
    return std::fabs(x - rhs.x) <= eps && std::fabs(y - rhs.y) <= eps && std::fabs(z - rhs.z) <= eps;
}

Vec3& Vec3::operator+=(const Vec3& rhs) noexcept
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

Vec3& Vec3::operator-=(const Vec3& rhs) noexcept
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

Vec3& Vec3::operator*=(float s) noexcept
{
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

Vec3& Vec3::operator/=(float s) noexcept
{
    const float inv = 1.0f / s;   // s가 0이면 Inf/Nan이 발생할 수 있음 — 필요 시 가드 추가
    x *= inv;
    y *= inv;
    z *= inv;
    return *this;
}
