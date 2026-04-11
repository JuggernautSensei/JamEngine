// ReSharper disable CppClangTidyModernizeUseStdNumbers
#pragma once
#include <random>
#include "TypeTraits.h"

namespace Jam
{

template<typename T>
struct Limits
{
};

template<>
struct Limits<bool>
{
    static constexpr bool k_min     = false;
    static constexpr bool k_max     = true;
    static constexpr bool k_highest = true;
    static constexpr bool k_lowest  = false;
};

template<>
struct Limits<double>
{
    static constexpr double k_min     = DBL_MIN;
    static constexpr double k_max     = DBL_MAX;
    static constexpr double k_highest = DBL_MAX;
    static constexpr double k_lowest  = -DBL_MAX;
};

template<>
struct Limits<float>
{
    static constexpr float k_min     = FLT_MIN;
    static constexpr float k_max     = FLT_MAX;
    static constexpr float k_highest = FLT_MAX;
    static constexpr float k_lowest  = -FLT_MAX;
};

template<>
struct Limits<Int32>
{
    static constexpr Int32 k_min     = INT32_MIN;
    static constexpr Int32 k_max     = INT32_MAX;
    static constexpr Int32 k_highest = INT32_MAX;
    static constexpr Int32 k_lowest  = INT32_MIN;
};

template<>
struct Limits<UInt32>
{
    static constexpr UInt32 k_min     = 0;
    static constexpr UInt32 k_max     = UINT32_MAX;
    static constexpr UInt32 k_highest = UINT32_MAX;
    static constexpr UInt32 k_lowest  = 0;
};

template<>
struct Limits<Int64>
{
    static constexpr Int64 k_min     = INT64_MIN;
    static constexpr Int64 k_max     = INT64_MAX;
    static constexpr Int64 k_highest = INT64_MAX;
    static constexpr Int64 k_lowest  = INT64_MIN;
};

template<>
struct Limits<UInt64>
{
    static constexpr UInt64 k_min     = 0;
    static constexpr UInt64 k_max     = UINT64_MAX;
    static constexpr UInt64 k_highest = UINT64_MAX;
    static constexpr UInt64 k_lowest  = 0;
};

template<>
struct Limits<UInt16>
{
    static constexpr UInt16 k_min     = 0;
    static constexpr UInt16 k_max     = UINT16_MAX;
    static constexpr UInt16 k_highest = UINT16_MAX;
    static constexpr UInt16 k_lowest  = 0;
};

template<>
struct Limits<Int16>
{
    static constexpr Int16 k_min     = INT16_MIN;
    static constexpr Int16 k_max     = INT16_MAX;
    static constexpr Int16 k_highest = INT16_MAX;
    static constexpr Int16 k_lowest  = INT16_MIN;
};

template<>
struct Limits<UInt8>
{
    static constexpr UInt8 k_min     = 0;
    static constexpr UInt8 k_max     = UINT8_MAX;
    static constexpr UInt8 k_highest = UINT8_MAX;
    static constexpr UInt8 k_lowest  = 0;
};

template<>
struct Limits<Int8>
{
    static constexpr Int8 k_min     = INT8_MIN;
    static constexpr Int8 k_max     = INT8_MAX;
    static constexpr Int8 k_highest = INT8_MAX;
    static constexpr Int8 k_lowest  = INT8_MIN;
};

// handle magic numbers
template<typename T>
struct Numbers
{
    static_assert(AlwaysFalseV<T>, "Numbers<T> is not defined for this type.");
};

template<>
struct Numbers<float>
{
    constexpr static float k_pi        = 3.1415926535897932f;
    constexpr static float k_2pi       = 2.f * k_pi;
    constexpr static float k_halfPi    = k_pi / 2.f;
    constexpr static float k_invPi     = 1.f / k_pi;
    constexpr static float k_inv2Pi    = 1.f / k_2pi;
    constexpr static float k_invHalfPi = 1.f / k_halfPi;
    constexpr static float k_e         = 2.7182818284590451f;
    constexpr static float k_epsilon   = 1e-6f;
    constexpr static float k_ln2       = 0.6931471805599453f;
    constexpr static float k_ln10      = 2.302585092994046f;
    constexpr static float k_sqrt2     = 1.4142135623730951f;
    constexpr static float k_toRad     = k_pi / 180.f;
    constexpr static float k_toDeg     = 180.f / k_pi;
};

template<>
struct Numbers<Vec2>
{

    constexpr static Vec2 k_left  = { -1.f, 0.f };
    constexpr static Vec2 k_right = { 1.f, 0.f };
    constexpr static Vec2 k_up    = { 0.f, 1.f };
    constexpr static Vec2 k_down  = { 0.f, -1.f };
    constexpr static Vec2 k_unitX = { 1.f, 0.f };
    constexpr static Vec2 k_unitY = { 0.f, 1.f };
    constexpr static Vec2 k_zero  = { 0.f, 0.f };
    constexpr static Vec2 k_one   = { 1.f, 1.f };
    constexpr static Vec2 k_max   = { Limits<float>::k_max, Limits<float>::k_max };
    constexpr static Vec2 k_min   = { Limits<float>::k_min, Limits<float>::k_min };
};

template<>
struct Numbers<Vec3>
{
    constexpr static Vec3 k_left     = { -1.f, 0.f, 0.f };
    constexpr static Vec3 k_right    = { 1.f, 0.f, 0.f };
    constexpr static Vec3 k_up       = { 0.f, 1.f, 0.f };
    constexpr static Vec3 k_down     = { 0.f, -1.f, 0.f };
    constexpr static Vec3 k_forward  = { 0.f, 0.f, 1.f };
    constexpr static Vec3 k_backward = { 0.f, 0.f, -1.f };
    constexpr static Vec3 k_unitX    = { 1.f, 0.f, 0.f };
    constexpr static Vec3 k_unitY    = { 0.f, 1.f, 0.f };
    constexpr static Vec3 k_unitZ    = { 0.f, 0.f, 1.f };
    constexpr static Vec3 k_zero     = { 0.f, 0.f, 0.f };
    constexpr static Vec3 k_one      = { 1.f, 1.f, 1.f };
    constexpr static Vec3 k_max      = { Limits<float>::k_max, Limits<float>::k_max, Limits<float>::k_max };
    constexpr static Vec3 k_min      = { Limits<float>::k_min, Limits<float>::k_min, Limits<float>::k_min };
};   // namespace Vec3M

template<>
struct Numbers<Vec4>
{
    constexpr static Vec4 k_left     = { -1.f, 0.f, 0.f, 0.f };
    constexpr static Vec4 k_right    = { 1.f, 0.f, 0.f, 0.f };
    constexpr static Vec4 k_up       = { 0.f, 1.f, 0.f, 0.f };
    constexpr static Vec4 k_down     = { 0.f, -1.f, 0.f, 0.f };
    constexpr static Vec4 k_forward  = { 0.f, 0.f, 1.f, 0.f };
    constexpr static Vec4 k_backward = { 0.f, 0.f, -1.f, 0.f };
    constexpr static Vec4 k_unitX    = { 1.f, 0.f, 0.f, 0.f };
    constexpr static Vec4 k_unitY    = { 0.f, 1.f, 0.f, 0.f };
    constexpr static Vec4 k_unitZ    = { 0.f, 0.f, 1.f, 0.f };
    constexpr static Vec4 k_unitW    = { 0.f, 0.f, 0.f, 1.f };
    constexpr static Vec4 k_zero     = { 0.f, 0.f, 0.f, 0.f };
    constexpr static Vec4 k_one      = { 1.f, 1.f, 1.f, 1.f };
    constexpr static Vec4 k_max      = { Limits<float>::k_max, Limits<float>::k_max, Limits<float>::k_max, Limits<float>::k_max };
    constexpr static Vec4 k_min      = { Limits<float>::k_min, Limits<float>::k_min, Limits<float>::k_min, Limits<float>::k_min };
};   // namespace Vec4M

template<>
struct Numbers<Quat>
{
    constexpr static Quat k_identity = { 0.f, 0.f, 0.f, 1.f };
};

template<>
struct Numbers<Mtx4>
{
    constexpr static Mtx4 k_identity = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
};

struct Vec2i
{
    static const Vec2i k_zero;
    static const Vec2i k_one;

    constexpr Vec2i() = default;
    constexpr Vec2i(const Int32 _x, const Int32 _y)
        : x(_x)
        , y(_y)
    {
    }

    constexpr Vec2i operator+(const Vec2i other) const
    {
        return Vec2i { x + other.x, y + other.y };
    }

    constexpr Vec2i operator-(const Vec2i other) const
    {
        return Vec2i { x - other.x, y - other.y };
    }

    constexpr Vec2i operator*(const Int32 scalar) const
    {
        return Vec2i { x * scalar, y * scalar };
    }

    constexpr Vec2i& operator+=(const Vec2i other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vec2i& operator-=(const Vec2i other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vec2i& operator*=(const Int32 scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Int32 x = 0;
    Int32 y = 0;
};

struct SinCos
{
    float sin = 0.f;
    float cos = 0.f;
};

template<typename To, typename From>
    requires(std::is_arithmetic_v<To>
             && std::is_arithmetic_v<From>
             && std::is_floating_point_v<To> == std::is_floating_point_v<From>)   // prevent float to int conversion
[[nodiscard]] constexpr To NarrowCast(From _value) noexcept
{
    if (JAM_DEBUG_ENABLED)
    {
        if constexpr (std::is_signed_v<To> == std::is_signed_v<From>)
        {
            JAM_ASSERT(static_cast<From>(static_cast<To>(_value)) == _value, "NarrowCast overflow: cannot cast {} to {}.", _value, static_cast<To>(_value));
        }
        else if constexpr (std::is_signed_v<From>)   // signed to unsigned
        {
            JAM_ASSERT(_value >= 0 && static_cast<From>(static_cast<To>(_value)) == _value, "NarrowCast overflow: cannot cast {} to {}.", _value, static_cast<To>(_value));
        }
        else   // unsigned to signed
        {
            JAM_ASSERT(std::cmp_greater(_value, Limits<To>::k_highest) == false, "NarrowCast overflow: cannot cast {} to {}.", _value, static_cast<To>(_value));
        }
    }

    return static_cast<To>(_value);
}

template<typename T>
[[nodiscard]] constexpr T Max(const T a, const std::type_identity_t<T> b)
{
    return a > b ? a : b;
}

template<typename T>
[[nodiscard]] constexpr T Min(const T a, const std::type_identity_t<T> b)
{
    return a < b ? a : b;
}

template<typename T, typename... R>
[[nodiscard]] constexpr T Max(const T a, const std::type_identity_t<T> b, const R... args)
{
    return Max(Max(a, b), args...);
}

template<typename T, typename... R>
[[nodiscard]] constexpr T Min(const T a, const std::type_identity_t<T> b, const R... args)
{
    return Min(Min(a, b), args...);
}

template<typename T>
[[nodiscard]] constexpr T Clamp(const T value, const T min, const T max)
{
    return Max(Min(value, max), min);
}

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr bool IsPowerOf2(const T value)
{
    return (value > 0) && ((value & (value - 1)) == 0);
}

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr T NextPowerOf2(const T value)
{
    if (value <= 0)
    {
        return 1;
    }

    using U = std::make_unsigned_t<T>;
    return static_cast<T>(std::bit_ceil(static_cast<U>(value)));
}

[[nodiscard]] constexpr float ToRad(const float degrees)
{
    return degrees * Numbers<float>::k_toRad;
}
[[nodiscard]] constexpr float ToDeg(const float radians)
{
    return radians * Numbers<float>::k_toDeg;
}

[[nodiscard]] constexpr UInt32 FloatToBits(const float _x)
{
    return std::bit_cast<UInt32>(_x);
}

[[nodiscard]] constexpr float BitsToFloat(const UInt32 _x)
{
    return std::bit_cast<float>(_x);
}

[[nodiscard]] constexpr float Sign(const float _x)
{
    return (_x > 0.f) ? 1.f : ((_x < 0.f) ? -1.f : 0.f);
}

[[nodiscard]] constexpr float Trunc(const float _x)
{
    return static_cast<float>(static_cast<Int64>(_x));
}

[[nodiscard]] constexpr float Fract(const float _x)
{
    return _x - Trunc(_x);
}

[[nodiscard]] constexpr float Floor(const float _x)
{
    if (_x < 0.f)
    {
        const float fr = Fract(-_x);
        const float tr = Trunc(-_x);
        return -tr - (fr != 0.f ? 1.f : 0.f);
    }
    else
    {
        return Trunc(_x);
    }
}

[[nodiscard]] constexpr float Ceil(const float _x)
{
    return -Floor(-_x);
}

[[nodiscard]] constexpr float Round(const float _x)
{
    return (_x >= 0.0f) ? Floor(_x + 0.5f) : Ceil(_x - 0.5f);
}

[[nodiscard]] constexpr float Lerp(const float _a, const float _b, const float _t)
{
    return _a + (_b - _a) * _t;
}

[[nodiscard]] constexpr float Square(const float _x)
{
    return _x * _x;
}

[[nodiscard]] constexpr float Abs(const float value)
{
    return value < 0.f ? -value : value;
}

[[nodiscard]] constexpr bool SignBit(const float _x)
{
    return -0.0f == _x ? 0.0f != _x : 0.0f > _x;
}

[[nodiscard]] constexpr float CopySign(const float _value, const float _sign)
{
    return SignBit(_value) != SignBit(_sign) ? -_value : _value;
}

[[nodiscard]] constexpr float Mod(const float _x, const float _y)
{
    return _x - _y * Floor(_x / _y);
}

[[nodiscard]] constexpr bool IsZeroApprox(const float _x, const float _epsilon = Numbers<float>::k_epsilon)
{
    return Abs(_x) < _epsilon;
}

[[nodiscard]] constexpr bool IsEqualApprox(const float _x, const float _y, const float _epsilon = Numbers<float>::k_epsilon)
{
    return Abs(_x - _y) < _epsilon;
}

[[nodiscard]] constexpr float Wrap(const float _x, const float _min, const float _max)
{
    const float range = _max - _min;
    if (IsZeroApprox(range))
    {
        return _min;
    }
    const float result = _x - (range * Floor((_x - _min) / range));
    if (IsEqualApprox(result, _max))
    {
        return _min;
    }
    return result;
}

[[nodiscard]] inline float Log(const float _x)
{
    return std::logf(_x);
}

[[nodiscard]] inline float Log2(const float _x)
{
    constexpr float k_invLogNat2 = 1.f / Numbers<float>::k_ln2;
    return Log(_x) * k_invLogNat2;
}

[[nodiscard]] inline float LdExp(const float _a, const Int32 _b)
{
    return std::ldexpf(_a, _b);
}

[[nodiscard]] inline float Exp(const float _x)
{
    return std::expf(_x);
}

[[nodiscard]] inline float Power(const float _x, const float _y)
{
    return std::powf(_x, _y);
}

[[nodiscard]] inline float Sqrt(const float _x)
{
    return std::sqrtf(_x);
}

[[nodiscard]] inline float InvSqrt(const float _x)
{
    return 1.0f / std::sqrtf(_x);
}

[[nodiscard]] inline float Cos(const float _x)
{
    return std::cosf(_x);
}

[[nodiscard]] inline float Sin(const float _x)
{
    return std::sinf(_x);
}

[[nodiscard]] inline float Tan(const float _x)
{
    return std::tanf(_x);
}

[[nodiscard]] inline float Acos(const float _a)
{
    return std::acosf(_a);
}

[[nodiscard]] inline float Asin(const float _a)
{
    return std::asinf(_a);
}

[[nodiscard]] inline float Atan2(const float _y, const float _x)
{
    return std::atan2f(_y, _x);
}

[[nodiscard]] inline float ATan(const float _x)
{
    return std::atanf(_x);
}

[[nodiscard]] constexpr Vec2 ToDeg(const Vec2 _radians)
{
    return { ToDeg(_radians.x), ToDeg(_radians.y) };
}

[[nodiscard]] constexpr Vec3 ToDeg(const Vec3 _radians)
{
    return { ToDeg(_radians.x), ToDeg(_radians.y), ToDeg(_radians.z) };
}

[[nodiscard]] constexpr Vec4 ToDeg(const Vec4 _radians)
{
    return { ToDeg(_radians.x), ToDeg(_radians.y), ToDeg(_radians.z), ToDeg(_radians.w) };
}

[[nodiscard]] constexpr Vec2 ToRad(const Vec2& _degrees)
{
    return { ToRad(_degrees.x), ToRad(_degrees.y) };
}

[[nodiscard]] constexpr Vec3 ToRad(const Vec3 _degrees)
{
    return { ToRad(_degrees.x), ToRad(_degrees.y), ToRad(_degrees.z) };
}

[[nodiscard]] constexpr Vec4 ToRad(const Vec4& _degrees)
{
    return { ToRad(_degrees.x), ToRad(_degrees.y), ToRad(_degrees.z), ToRad(_degrees.w) };
}

[[nodiscard]] constexpr Vec2 Add(const Vec2 a, const Vec2 b)
{
    return { a.x + b.x, a.y + b.y };
}

[[nodiscard]] constexpr Vec2 Sub(const Vec2 a, const Vec2 b)
{
    return { a.x - b.x, a.y - b.y };
}

[[nodiscard]] constexpr Vec3 Add(const Vec3 a, const Vec3 b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

[[nodiscard]] constexpr Vec3 Sub(const Vec3 a, const Vec3 b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

[[nodiscard]] constexpr Vec4 Add(const Vec4 a, const Vec4 b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

[[nodiscard]] constexpr Vec4 Sub(const Vec4 a, const Vec4 b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

[[nodiscard]] constexpr float Dot(const Vec2 a, const Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

[[nodiscard]] constexpr float Dot(const Vec3 a, const Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] constexpr float Dot(const Vec4 a, const Vec4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

[[nodiscard]] constexpr Vec3 Cross(const Vec3 a, const Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

[[nodiscard]] constexpr float LengthSq(const Vec2 vec)
{
    return Dot(vec, vec);
}

[[nodiscard]] constexpr float LengthSq(const Vec3 vec)
{
    return Dot(vec, vec);
}

[[nodiscard]] constexpr float LengthSq(const Vec4 vec)
{
    return Dot(vec, vec);
}

[[nodiscard]] inline float Length(const Vec2 vec)
{
    return Sqrt(LengthSq(vec));
}

[[nodiscard]] inline float Length(const Vec3 vec)
{
    return Sqrt(LengthSq(vec));
}

[[nodiscard]] inline float Length(const Vec4 vec)
{
    return Sqrt(LengthSq(vec));
}

[[nodiscard]] inline float InvLength(const Vec2 vec)
{
    return InvSqrt(LengthSq(vec));
}

[[nodiscard]] inline float InvLength(const Vec3 vec)
{
    return InvSqrt(LengthSq(vec));
}

[[nodiscard]] inline float InvLength(const Vec4 vec)
{
    return InvSqrt(LengthSq(vec));
}

[[nodiscard]] constexpr float DistanceSq(const Vec2 a, const Vec2 b)
{
    return LengthSq(Sub(b, a));
}

[[nodiscard]] constexpr float DistanceSq(const Vec3 a, const Vec3 b)
{
    return LengthSq(Sub(b, a));
}

[[nodiscard]] constexpr float DistanceSq(const Vec4 a, const Vec4 b)
{
    return LengthSq(Sub(b, a));
}

[[nodiscard]] inline float Distance(const Vec2 a, const Vec2 b)
{
    return Length(Sub(b, a));
}

[[nodiscard]] inline float Distance(const Vec3 a, const Vec3 b)
{
    return Length(Sub(b, a));
}

[[nodiscard]] inline float Distance(const Vec4 a, const Vec4 b)
{
    return Length(Sub(b, a));
}

[[nodiscard]] constexpr Vec2 Vec2From(Vec3 a)
{
    return { a.x, a.y };
}

[[nodiscard]] constexpr Vec2 Vec2From(Vec4 a)
{
    return { a.x, a.y };
}

[[nodiscard]] constexpr Vec3 Vec3From(Vec2 a, float z = 0.f)
{
    return { a.x, a.y, z };
}

[[nodiscard]] constexpr Vec3 Vec3From(Vec4 a)
{
    return { a.x, a.y, a.z };
}

[[nodiscard]] constexpr Vec4 Vec4From(Vec2 a, float z = 0.f, float w = 0.f)
{
    return { a.x, a.y, z, w };
}

[[nodiscard]] constexpr Vec4 Vec4From(Vec3 a, float w = 0.f)
{
    return { a.x, a.y, a.z, w };
}

[[nodiscard]] inline Vec2 Normalized(const Vec2 vec)
{
    const float lenSq = LengthSq(vec);
    if (IsZeroApprox(lenSq))
    {
        return Numbers<Vec2>::k_zero;
    }
    return vec * InvSqrt(lenSq);
}

[[nodiscard]] inline Vec3 Normalized(const Vec3 vec)
{
    const float lenSq = LengthSq(vec);
    if (IsZeroApprox(lenSq))
    {
        return Numbers<Vec3>::k_zero;
    }
    return vec * InvSqrt(lenSq);
}

[[nodiscard]] inline Vec4 Normalized(const Vec4 vec)
{
    const float lenSq = LengthSq(vec);
    if (IsZeroApprox(lenSq))
    {
        return Numbers<Vec4>::k_zero;
    }
    return vec * InvSqrt(lenSq);
}

[[nodiscard]] constexpr Vec2 Lerp(const Vec2 _v0, const Vec2 _v1, const float t)
{
    return Add(_v0, Sub(_v1, _v0) * t);
}

[[nodiscard]] constexpr Vec3 Lerp(const Vec3 _v0, const Vec3 _v1, const float t)
{
    return Add(_v0, Sub(_v1, _v0) * t);
}

[[nodiscard]] constexpr Vec4 Lerp(const Vec4 _v0, const Vec4 _v1, const float t)
{
    return Add(_v0, Sub(_v1, _v0) * t);
}

[[nodiscard]] constexpr bool IsNormalized(const Vec2 vec, const float epsilon = Numbers<float>::k_epsilon)
{
    return IsEqualApprox(LengthSq(vec), 1.f, epsilon * epsilon);
}
[[nodiscard]] constexpr bool IsNormalized(const Vec3 vec, const float epsilon = Numbers<float>::k_epsilon)
{
    return IsEqualApprox(LengthSq(vec), 1.f, epsilon * epsilon);
}
[[nodiscard]] constexpr bool IsNormalized(const Vec4 vec, const float epsilon = Numbers<float>::k_epsilon)
{
    return IsEqualApprox(LengthSq(vec), 1.f, epsilon * epsilon);
}

[[nodiscard]] constexpr bool IsZeroApprox(const Vec2 vec, const float _epsilon = Numbers<float>::k_epsilon)
{
    return IsZeroApprox(vec.x, _epsilon) && IsZeroApprox(vec.y, _epsilon);
}

[[nodiscard]] constexpr bool IsZeroApprox(const Vec3 vec, const float _epsilon = Numbers<float>::k_epsilon)
{
    return IsZeroApprox(vec.x, _epsilon) && IsZeroApprox(vec.y, _epsilon) && IsZeroApprox(vec.z, _epsilon);
}

[[nodiscard]] constexpr bool IsZeroApprox(const Vec4 vec, const float _epsilon = Numbers<float>::k_epsilon)
{
    return IsZeroApprox(vec.x, _epsilon) && IsZeroApprox(vec.y, _epsilon) && IsZeroApprox(vec.z, _epsilon) && IsZeroApprox(vec.w, _epsilon);
}

[[nodiscard]] constexpr bool IsEqualApprox(const Vec2 a, const Vec2 b, const float epsilon = Numbers<float>::k_epsilon)
{
    return IsEqualApprox(a.x, b.x, epsilon) && IsEqualApprox(a.y, b.y, epsilon);
}

[[nodiscard]] constexpr bool IsEqualApprox(const Vec3 a, const Vec3 b, const float epsilon = Numbers<float>::k_epsilon)
{
    return IsEqualApprox(a.x, b.x, epsilon) && IsEqualApprox(a.y, b.y, epsilon) && IsEqualApprox(a.z, b.z, epsilon);
}

[[nodiscard]] constexpr bool IsEqualApprox(const Vec4 a, const Vec4 b, const float epsilon = Numbers<float>::k_epsilon)
{
    return IsEqualApprox(a.x, b.x, epsilon) && IsEqualApprox(a.y, b.y, epsilon) && IsEqualApprox(a.z, b.z, epsilon) && IsEqualApprox(a.w, b.w, epsilon);
}

[[nodiscard]] constexpr Vec2 Abs(const Vec2& vec)
{
    return { Abs(vec.x), Abs(vec.y) };
}

[[nodiscard]] constexpr Vec3 Abs(const Vec3 vec)
{
    return { Abs(vec.x), Abs(vec.y), Abs(vec.z) };
}

[[nodiscard]] constexpr Vec4 Abs(const Vec4& vec)
{
    return { Abs(vec.x), Abs(vec.y), Abs(vec.z), Abs(vec.w) };
}

[[nodiscard]] Float16 ToFloat16(float value);
[[nodiscard]] float   ToFloat32(Float16 value);

[[nodiscard]] Vec3 EulerFrom(Vec3 _forward);      // euler: pitch, yaw, roll
[[nodiscard]] Vec3 ForwardFrom(Vec3 _eulerRad);   // euler: pitch, yaw, roll

[[nodiscard]] Mtx4 CreateOrthoMatrix(float width, float height, float nearZ, float farZ);
[[nodiscard]] Mtx4 CreateOrthoMatrix(float left, float right, float bottom, float top, float nearZ, float farZ);
[[nodiscard]] Mtx4 CreatePerspectiveMatrix(float fovYRad, float aspect, float zNear, float zFar);

namespace Details
{
    extern std::mt19937 g_randomGenerator;
    extern UInt32       g_randomSeed;
}   // namespace Details

template<typename T>
[[nodiscard]] T MakeRandom(
    const T      _min  = Limits<T>::k_min,
    const T      _max  = Limits<T>::k_max,
    const UInt32 _seed = std::mt19937::default_seed)
{
    if (_seed != Details::g_randomSeed)
    {
        Details::g_randomGenerator.seed(_seed);
        Details::g_randomSeed = _seed;
    }

    if constexpr (std::is_integral_v<T>)
    {
        std::uniform_int_distribution<T> dist(_min, _max);
        return dist(Details::g_randomGenerator);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        std::uniform_real_distribution<T> dist(_min, _max);
        return dist(Details::g_randomGenerator);
    }
    else
    {
        static_assert(AlwaysFalseV<T>, "MakeRandom: ByteT must be an integral or floating-point type.");
        return T {};
    }
}

}   // namespace Jam
