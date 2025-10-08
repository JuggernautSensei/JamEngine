#pragma once

struct Vec3
{
    float x { 0.f }, y { 0.f }, z { 0.f };

    // 생성자
    Vec3() = default;
    constexpr Vec3(float _x, float _y, float _z) noexcept
        : x(_x)
        , y(_y)
        , z(_z)
    {
    }

    // 길이/정규화
    float length() const noexcept;
    float length_sq() const noexcept;
    Vec3  normalized() const noexcept;
    void  normalize() noexcept;

    // 유틸
    bool is_finite() const noexcept;
    bool approx_equal(const Vec3& rhs, float eps = 1e-6f) const noexcept;

    // 멤버 연산(복합 대입)
    Vec3& operator+=(const Vec3& rhs) noexcept;
    Vec3& operator-=(const Vec3& rhs) noexcept;
    Vec3& operator*=(float s) noexcept;
    Vec3& operator/=(float s) noexcept;
};
