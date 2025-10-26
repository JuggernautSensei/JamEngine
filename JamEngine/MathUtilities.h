#pragma once
#include "TypeTrait.h"
#include <numbers>
#include <random>

namespace jam
{

//////////////////////////////////
///          CONSTANT          ///
//////////////////////////////////

constexpr float  k_pi                = std::numbers::pi_v<float>;       // pi constant
constexpr float  k_2pi               = 2.f * k_pi;                      // 2 * pi constant
constexpr float  k_piDiv2            = k_pi / 2.f;                      // pi / 2 constant
constexpr float  k_piDiv4            = k_pi / 4.f;                      // pi / 4 constant
constexpr float  k_1DivPi            = std::numbers::inv_pi_v<float>;   // 1 / pi constant
constexpr float  k_1Div2Pi           = 1.f / (2.f * k_pi);              // 1 / (2 * pi) constant
constexpr float  k_epsilon           = 1e-6f;                           // epsilon constant for floating point comparison
constexpr float  k_radToDeg          = 180.f / k_pi;                    // radian to degree conversion factor
constexpr float  k_degToRad          = k_pi / 180.f;                    // degree to radian conversion factor
constexpr UInt32 k_defaultRandomSeed = 5489U;                           // default seed for random number m_token

using FloatLimits  = std::numeric_limits<float>;
using DoubleLimits = std::numeric_limits<double>;
using Int16Limits  = std::numeric_limits<Int16>;
using UInt16Limits = std::numeric_limits<UInt16>;
using Int32Limits  = std::numeric_limits<Int32>;
using UInt32Limits = std::numeric_limits<UInt32>;
using Int64Limits  = std::numeric_limits<Int64>;
using UInt64Limits = std::numeric_limits<UInt64>;

/////////////////////////////////////////
///          BASIC UTILITIES          ///
/////////////////////////////////////////

/// @brief 절댓값
template<typename T>
JAM_NODISCARD constexpr T Absolute(const T value)
{
    return value < static_cast<T>(0) ? -value : value;
}

/// @brief 거의 0인지 비교 (부동소수점 오차 범위 내)
template<typename T>
    requires std::is_floating_point_v<T>
JAM_NODISCARD constexpr bool IsNearlyZero(const T value)
{
    return Absolute(value) < k_epsilon;
}

/// @brief 거의 같은지 비교 (부동소수점 오차 범위 내)
template<typename T>
    requires std::is_floating_point_v<T>
JAM_NODISCARD constexpr bool IsNearlyEqual(const T a, const T b)
{
    return Absolute(a - b) < k_epsilon;
}
/// @brief 올림
template<typename T>
    requires std::is_floating_point_v<T>
JAM_NODISCARD constexpr T Ceil(const T value)
{
    const long long trunc = static_cast<long long>(value);
    return (value > static_cast<T>(trunc)) ? static_cast<T>(trunc + 1)
                                           : static_cast<T>(trunc);
}

/// @brief 내림
template<typename T>
    requires std::is_floating_point_v<T>
JAM_NODISCARD constexpr T Floor(const T value)
{
    const long long trunc = static_cast<long long>(value);
    return (value < 0 && value != static_cast<T>(trunc)) ? static_cast<T>(trunc - 1)
                                                         : static_cast<T>(trunc);
}

/// @brief 나눗셈
template<class T>
constexpr T Mod(const T x, const T y)
{
    if constexpr (std::is_floating_point_v<T>)
    {
        return x - static_cast<long long>(x / y) * y;
    }
    else
    {
        return x % y;
    }
}

/// @brief 선형보간
template<typename T>
constexpr T Lerp(const T a, const T b, const float t)
{
    return a + (b - a) * t;
}

/// @brief 제한
template<typename T>
    requires std::is_arithmetic_v<T>
JAM_NODISCARD constexpr T Clamp(const T value, const T min, const T max)
{
    return value < min ? min
         : value > max ? max
                       : value;
}

/// @brief 순환
template<typename T>
constexpr T Wrap(T value, T min, T max)
{
    const T range = max - min;
    if (range == 0)
    {
        return std::numeric_limits<T>::quiet_NaN();
    }

    T result = Mod(value - min, range);
    if (result < 0)
    {
        result += range;
    }

    return result + min;
}

template<typename T, typename... R>
JAM_NODISCARD constexpr T Min(const T a, const R... args)
{
    T m = a;
    ((m = (m < static_cast<T>(args) ? m : static_cast<T>(args))), ...);
    return m;
}

template<typename T, typename... R>
JAM_NODISCARD constexpr T Max(const T a, const R... args)
{
    T M = a;
    ((M = (M > static_cast<T>(args) ? M : static_cast<T>(args))), ...);
    return M;
}

JAM_NODISCARD constexpr Int32 Log2_Int32(Int32 value)
{
    const Int32 k_table[32] = {
        0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
    };

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return k_table[static_cast<Int32>(value * 0x07C4ACDD) >> 27];
}

namespace details
{
    template<typename T>
        requires std::is_floating_point_v<T>
    JAM_NODISCARD T constexpr SqrtHelper(const T x, const T curr, const T prev)
    {
        return IsNearlyEqual(curr, prev)
                 ? curr
                 : SqrtHelper(x, static_cast<T>(0.5) * (curr + x / curr), curr);
    }
}   // namespace details

/// @brief 컴파일타임 루트 2
template<typename T>
    requires std::is_floating_point_v<T>
JAM_NODISCARD constexpr T Sqrt(const T x)
{
    return x >= 0.f && x < std::numeric_limits<T>::infinity()
             ? details::SqrtHelper(x, x, static_cast<T>(0))
             : std::numeric_limits<T>::quiet_NaN();
}

/// @brief 컴파일타임 제곱
template<typename T>
    requires std::is_arithmetic_v<T>
JAM_NODISCARD constexpr T Power(const T base, const Int32 exponent)
{
    return exponent < 0  ? static_cast<T>(1) / Power(base, -exponent)
         : exponent == 0 ? static_cast<T>(1)
                         : base * Power(base, exponent - 1);
}

///////////////////////////////////////
///          TRIGONOMETRIC          ///
///////////////////////////////////////

JAM_NODISCARD constexpr float ToRad(const float degrees)
{
    return degrees * k_degToRad;
}

JAM_NODISCARD constexpr float ToDeg(const float radians)
{
    return radians * k_radToDeg;
}

/// @brief 코사인과 사인을 동시에 계산합니다. (테일러 급수 사용)
/// @return std::pair<sin, cos>
struct SinCos
{
    float sin;
    float cos;
};

JAM_NODISCARD constexpr SinCos ComputeSinCos(const float _radian)
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

////////////////////////////////
///          VECTOR          ///
////////////////////////////////

JAM_NODISCARD constexpr Vec3 Absolute(const Vec3& vec)
{
    return { Absolute(vec.x), Absolute(vec.y), Absolute(vec.z) };
}

JAM_NODISCARD constexpr bool IsNearlyEqual(const Vec2& a, const Vec2& b)
{
    return IsNearlyEqual(a.x, b.x) && IsNearlyEqual(a.y, b.y);
}

JAM_NODISCARD constexpr bool IsNearlyEqual(const Vec3& a, const Vec3& b)
{
    return IsNearlyEqual(a.x, b.x) && IsNearlyEqual(a.y, b.y) && IsNearlyEqual(a.z, b.z);
}

JAM_NODISCARD constexpr bool IsNearlyEqual(const Vec4& a, const Vec4& b)
{
    return IsNearlyEqual(a.x, b.x) && IsNearlyEqual(a.y, b.y) && IsNearlyEqual(a.z, b.z) && IsNearlyEqual(a.w, b.w);
}

JAM_NODISCARD constexpr bool IsZeroVector(const Vec2& vec)
{
    return IsNearlyZero(vec.LengthSquared());
}

JAM_NODISCARD constexpr bool IsZeroVector(const Vec3& vec)
{
    return IsNearlyZero(vec.LengthSquared());
}

JAM_NODISCARD constexpr bool IsZeroVector(const Vec4& vec)
{
    return IsNearlyZero(vec.LengthSquared());
}

JAM_NODISCARD constexpr Vec2 ToVec2(const Vec3& vec3)
{
    return { vec3.x, vec3.y };
}

JAM_NODISCARD constexpr Vec2 ToVec2(const Vec4& vec4)
{
    return { vec4.x, vec4.y };
}

JAM_NODISCARD constexpr Vec3 ToVec3(const Vec2& vec2, const float z = 0.f)
{
    return { vec2.x, vec2.y, z };
}

JAM_NODISCARD constexpr Vec3 ToVec3(const Vec4& vec4)
{
    return { vec4.x, vec4.y, vec4.z };
}

JAM_NODISCARD constexpr Vec4 ToVec4(const Vec2& vec2, const float z = 0.f, const float w = 1.f)
{
    return { vec2.x, vec2.y, z, w };
}

JAM_NODISCARD constexpr Vec4 ToVec4(const Vec3& vec3, const float w = 1.f)
{
    return { vec3.x, vec3.y, vec3.z, w };
}

////////////////////////////////
///          MATRIX          ///
////////////////////////////////

JAM_NODISCARD Mat4 CreateRotationLookTo(const Vec3& _forward);

JAM_NODISCARD Mat4 CreateWorld(const Vec3& _translate, const Vec3& _forward, const Vec3& _scale);

JAM_NODISCARD Mat4 CreateWorld(const Vec3& _translate, const Quat& _quat, const Vec3& _scale);

/// @brief LookTo 뷰 행렬 생성 (좌측 수직 좌표계)
JAM_NODISCARD Mat4 CreateViewLookTo(const Vec3& _position, const Vec3& _forward);

/// @brief LookAt 뷰 행렬 생성 (좌측 수직 좌표계)
JAM_NODISCARD Mat4 CreateViewLookAt(const Vec3& _position, const Vec3& _focusPoint);

/// @brief 직교 프로젝션 행렬 생성 (좌측 수직 좌표계)
JAM_NODISCARD Mat4 CreateOrthographic(float width, float height, float nearZ, float farZ);

/// @brief 직교 프로젝션 행렬 생성 (좌측 수직 좌표계)
JAM_NODISCARD Mat4 CreateOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ);

/// @brief 원근 프로젝션 행렬 생성 (좌측 수직 좌표계)
JAM_NODISCARD Mat4 CreatePerspectiveFov(float fovYRad, float aspectRatio, float nearZ, float farZ);

////////////////////////////////////
///          QUATERNION          ///
////////////////////////////////////

/// @brief 쿼터니언에서 축과 각도로 변환
/// @return std::pair<회전 축, 회전 각(라디안)
struct AxisAngle
{
    Vec3  axis;
    float angleRad;
};
JAM_NODISCARD AxisAngle QuatToAxisAngle(const Quat& _q);

////////////////////////////////
///          RANDOM          ///
////////////////////////////////

namespace details
{
    extern std::mt19937 g_randomGenerator;
    extern UInt32       g_randomSeed;
}   // namespace details

/// @brief 주어진 범위 [_min, _max] 내에서 난수 생성. (정수형 및 실수형 지원)
/// @tparam T 정수형 또는 실수형
/// @param _min
/// @param _max
/// @param _seed 난수 생성기 시드 (기본값: 5489U)
/// @return _min 이상 _max 이하의 난수
template<typename T>
JAM_NODISCARD T CreateRandom(T _min, T _max, const UInt32 _seed = std::mt19937::default_seed)
{
    if (_seed != details::g_randomSeed)
    {
        details::g_randomGenerator.seed(_seed);
        details::g_randomSeed = _seed;
    }

    if constexpr (std::is_integral_v<T>)
    {
        std::uniform_int_distribution<T> distribution(_min, _max);
        return distribution(details::g_randomGenerator);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        std::uniform_real_distribution<T> distribution(_min, _max);
        return distribution(details::g_randomGenerator);
    }
    else
    {
        static_assert(AlwaysFalse, "CreateRandom: T must be an integral or floating-point type.");
        return T {};
    }
}

}   // namespace jam