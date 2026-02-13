#pragma once
#include <random>

#include "TypeTraits.h"

namespace Jam
{

constexpr float  k_pi                    = 3.1415926535897932f;
constexpr float  k_2pi                   = 2.f * k_pi;
constexpr float  k_halfPi                = k_pi / 2.f;
constexpr float  k_invPi                 = 1.f / k_pi;
constexpr float  k_inv2Pi                = 1.f / k_2pi;
constexpr float  k_invHalfPi             = 1.f / k_halfPi;
constexpr float  k_e                     = 2.7182818284590451f;
constexpr float  k_epsilon               = 1e-5f;
constexpr float  k_ln2                   = 0.6931471805599453f;
constexpr float  k_ln10                  = 2.302585092994046f;
constexpr float  k_sqrt2                 = 1.4142135623730951f;
constexpr float  k_toRad                 = k_pi / 180.f;
constexpr float  k_toDeg                 = 180.f / k_pi;
constexpr UInt32 k_floatExponentMask     = UINT32_C(0x7f800000);
constexpr UInt32 k_floatMantissaMask     = UINT32_C(0x007fffff);
constexpr UInt32 k_floatSignMask         = UINT32_C(0x80000000);
constexpr UInt32 k_floatExponentBitShift = 23;
constexpr float  k_inf                   = std::bit_cast<float>(k_floatExponentMask);

// vec2 vec3 vec4 quat mtx4 magic numbers
// use this instead of DirectX::SimpleMath's magic numbers
// because not matching my application coordinate system

namespace vec2
{
    constexpr Vec2 k_left  = Vec2(-1.f, 0.f);
    constexpr Vec2 k_right = Vec2(1.f, 0.f);
    constexpr Vec2 k_up    = Vec2(0.f, 1.f);
    constexpr Vec2 k_down  = Vec2(0.f, -1.f);
    constexpr Vec2 k_unitX = Vec2(1.f, 0.f);
    constexpr Vec2 k_unitY = Vec2(0.f, 1.f);
    constexpr Vec2 k_zero  = Vec2(0.f, 0.f);
    constexpr Vec2 k_one   = Vec2(1.f, 1.f);
}   // namespace vec2

namespace vec3
{
    constexpr Vec3 k_left     = Vec3(-1.f, 0.f, 0.f);
    constexpr Vec3 k_right    = Vec3(1.f, 0.f, 0.f);
    constexpr Vec3 k_up       = Vec3(0.f, 1.f, 0.f);
    constexpr Vec3 k_down     = Vec3(0.f, -1.f, 0.f);
    constexpr Vec3 k_forward  = Vec3(0.f, 0.f, 1.f);
    constexpr Vec3 k_backward = Vec3(0.f, 0.f, -1.f);
    constexpr Vec3 k_unitX    = Vec3(1.f, 0.f, 0.f);
    constexpr Vec3 k_unitY    = Vec3(0.f, 1.f, 0.f);
    constexpr Vec3 k_unitZ    = Vec3(0.f, 0.f, 1.f);
    constexpr Vec3 k_zero     = Vec3(0.f, 0.f, 0.f);
    constexpr Vec3 k_one      = Vec3(1.f, 1.f, 1.f);
}   // namespace vec3

namespace vec4
{
    constexpr Vec4 k_left     = Vec4(-1.f, 0.f, 0.f, 0.f);
    constexpr Vec4 k_right    = Vec4(1.f, 0.f, 0.f, 0.f);
    constexpr Vec4 k_up       = Vec4(0.f, 1.f, 0.f, 0.f);
    constexpr Vec4 k_down     = Vec4(0.f, -1.f, 0.f, 0.f);
    constexpr Vec4 k_forward  = Vec4(0.f, 0.f, 1.f, 0.f);
    constexpr Vec4 k_backward = Vec4(0.f, 0.f, -1.f, 0.f);
    constexpr Vec4 k_unitX    = Vec4(1.f, 0.f, 0.f, 0.f);
    constexpr Vec4 k_unitY    = Vec4(0.f, 1.f, 0.f, 0.f);
    constexpr Vec4 k_unitZ    = Vec4(0.f, 0.f, 1.f, 0.f);
    constexpr Vec4 k_unitW    = Vec4(0.f, 0.f, 0.f, 1.f);
    constexpr Vec4 k_zero     = Vec4(0.f, 0.f, 0.f, 0.f);
    constexpr Vec4 k_one      = Vec4(1.f, 1.f, 1.f, 1.f);
}   // namespace vec4

namespace quat
{
    constexpr Quat k_identity = Quat(0.f, 0.f, 0.f, 1.f);
}

namespace mtx4
{
    constexpr Mtx4 k_identity = Mtx4 { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };
}

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
    requires std::is_arithmetic_v<To> && std::is_arithmetic_v<From>
[[nodiscard]] constexpr bool CheckOverflow(From _value) noexcept   // do not check float precision loss, only check if value is out of range of target type
{
    if constexpr (std::is_floating_point_v<From> && std::is_floating_point_v<To>)
    {
        return CmpLess(_value, Limits<To>::k_min) || CmpGreater(_value, Limits<To>::k_max);
    }
    else if constexpr (std::is_integral_v<From> && std::is_integral_v<To>)
    {
        if constexpr (std::is_signed_v<To> == std::is_signed_v<From>)
        {
            return static_cast<From>(static_cast<To>(_value)) != _value;
        }
        else if constexpr (std::is_signed_v<From>)
        {
            // signed to unsigned
            return _value < 0 || static_cast<From>(static_cast<To>(_value)) != _value;
        }
        else
        {
            // unsigned to signed
            return CmpGreater(_value, Limits<To>::k_max);
        }
    }
    else if constexpr (std::is_floating_point_v<From>)
    {
        // float to integral
        return CmpLess(_value, Limits<To>::k_min) || CmpGreater(_value, Limits<To>::k_max);
    }
    else
    {
        // integral to float
        return false;   // always no overflow. but may cause precision loss, which is not checked by this function
    }
}

template<typename To, typename From>
    requires std::is_arithmetic_v<To> && std::is_arithmetic_v<From>
[[nodiscard]] constexpr To NumericCast(From _value) noexcept
{
    JAM_ASSERT(!CheckOverflow<To>(_value), "NumericCast overflow: cannot cast value {} to target type.", _value);
    return static_cast<To>(_value);
}

template<typename To, typename From>
    requires std::is_arithmetic_v<To> && std::is_enum_v<From>
[[nodiscard]] constexpr To NumericCast(From _value) noexcept
{
    using UnderlyingT = std::underlying_type_t<From>;
    return NumericCast<To>(static_cast<UnderlyingT>(_value));
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
    return degrees * k_toRad;
}
[[nodiscard]] constexpr float ToDeg(const float radians)
{
    return radians * k_toDeg;
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
    return NumericCast<float>(NumericCast<Int32>(_x));
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
    return Floor(_x + 0.5f);
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

[[nodiscard]] constexpr bool IsZeroApprox(const float _x, const float _epsilon = k_epsilon)
{
    return Abs(_x) < _epsilon;
}

[[nodiscard]] constexpr bool IsEqualApprox(const float _x, const float _y, const float _epsilon = k_epsilon)
{
    return Abs(_x - _y) < _epsilon;
}

template<typename T>
[[nodiscard]] constexpr T Div(const T num, const T denom)
{
    if constexpr (std::is_floating_point_v<T>)
    {
        JAM_ASSERT(IsZeroApprox(denom) == false, "Division by zero");
    }
    else if constexpr (std::is_integral_v<T>)
    {
        JAM_ASSERT(denom != 0, "Division by zero");
    }
    else
    {
        static_assert(AlwaysFalseV<T>, "Div only supports integral and floating point types");
    }

    return num / denom;
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

[[nodiscard]] constexpr float Log(const float _x)
{
    if (_x < 0.0f)
    {
        return BitsToFloat(k_floatSignMask | k_floatExponentMask | k_floatMantissaMask);
    }

    if (_x == 0.0f)
    {
        return -k_inf;
    }

    const UInt32 ftob = FloatToBits(_x);

    const UInt32 masked0 = ftob & k_floatExponentMask;
    const UInt32 exp0    = masked0 >> k_floatExponentBitShift;
    Int32        exp     = static_cast<Int32>(exp0 - 0x7e);

    const UInt32 masked1 = ftob & (k_floatSignMask | k_floatMantissaMask);
    const UInt32 bits    = masked1 | UINT32_C(0x3f000000);
    float        ff      = BitsToFloat(bits);

    if (ff < k_sqrt2 * 0.5f)
    {
        ff *= 2.0f;
        --exp;
    }

    constexpr float kLogC0     = 6.666666666666735130e-01f;
    constexpr float kLogC1     = 3.999999999940941908e-01f;
    constexpr float kLogC2     = 2.857142874366239149e-01f;
    constexpr float kLogC3     = 2.222219843214978396e-01f;
    constexpr float kLogC4     = 1.818357216161805012e-01f;
    constexpr float kLogC5     = 1.531383769920937332e-01f;
    constexpr float kLogC6     = 1.479819860511658591e-01f;
    constexpr float kLogNat2Lo = 1.90821492927058770002e-10f;

    ff -= 1.0f;
    const float kk = NumericCast<float>(exp);
    const float hi = kk * k_ln2;
    const float lo = kk * kLogNat2Lo;
    const float ss = ff / (2.0f + ff);
    const float s2 = ss * ss;
    const float s4 = s2 * s2;

    const float tmp0 = kLogC6 * s4 + kLogC4;
    const float tmp1 = tmp0 * s4 + kLogC2;
    const float tmp2 = tmp1 * s4 + kLogC0;
    const float t1   = s2 * tmp2;

    const float tmp3 = kLogC5 * s4 + kLogC3;
    const float tmp4 = tmp3 * s4 + kLogC1;
    const float t2   = s4 * tmp4;

    const float t12    = t1 + t2;
    const float hfsq   = 0.5f * (ff * ff);
    const float result = hi - ((hfsq - (ss * (hfsq + t12) + lo)) - ff);

    return result;
}

[[nodiscard]] constexpr float Log2(const float _x)
{
    constexpr float k_invLogNat2 = 1.f / k_ln2;
    return Log(_x) * k_invLogNat2;
}

[[nodiscard]] constexpr UInt32 Log2(const UInt32 _x)
{
    constexpr UInt32 k_log2Table[32] = {
        0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
    };

    UInt32 result = _x;
    result |= result >> 1;
    result |= result >> 2;
    result |= result >> 4;
    result |= result >> 8;
    result |= result >> 16;
    return k_log2Table[(result * 0x07C4ACDD) >> 27];
}

[[nodiscard]] constexpr Int32 Log2(const Int32 _x)
{
    JAM_ASSERT(_x > 0, "Log2 is only defined for positive integers.");
    return NumericCast<Int32>(Log2(NumericCast<UInt32>(_x)));
}

[[nodiscard]] constexpr UInt64 Log2(const UInt64 _x)
{
    constexpr UInt32 k_log2Table[64] = {
        63, 0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56, 45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5
    };

    UInt64 result = _x;
    result |= result >> 1;
    result |= result >> 2;
    result |= result >> 4;
    result |= result >> 8;
    result |= result >> 16;
    result |= result >> 32;
    return k_log2Table[((result - (result >> 1)) * 0x07EDD5E59A4E28C2) >> 58];
}

[[nodiscard]] constexpr Int64 Log2(const Int64 _x)
{
    return NumericCast<Int64>(Log2(NumericCast<UInt64>(_x)));
}

[[nodiscard]] constexpr bool IsInfinite(const float _x)
{
    const UInt32 tmp = FloatToBits(_x) & Limits<Int32>::k_max;
    return tmp == k_floatExponentMask;
}

[[nodiscard]] constexpr float LdExp(const float _a, const Int32 _b)
{
    const UInt32 ftob     = FloatToBits(_a);
    const UInt32 masked   = ftob & (k_floatSignMask | k_floatExponentMask);
    const UInt32 expsign0 = masked >> k_floatExponentBitShift;
    const Int32  tmp      = static_cast<Int32>(expsign0) + _b;
    const UInt32 expsign1 = static_cast<UInt32>(tmp) << k_floatExponentBitShift;
    const UInt32 mantissa = ftob & k_floatMantissaMask;
    const UInt32 bits     = mantissa | expsign1;
    const float  result   = BitsToFloat(bits);
    return result;
}

[[nodiscard]] constexpr float Exp(const float _x)
{
    if (IsZeroApprox(_x))
    {
        return _x + 1.0f;
    }

    const float expMin = Log(Limits<float>::k_min);

    if (_x <= expMin)
    {
        return 0.0f;
    }

    if (IsInfinite(_x) && _x < 0.0f)
    {
        return 0.0f;
    }

    constexpr float kExpC0     = 1.66666666666666019037e-01f;
    constexpr float kExpC1     = -2.77777777770155933842e-03f;
    constexpr float kExpC2     = 6.61375632143793436117e-05f;
    constexpr float kExpC3     = -1.65339022054652515390e-06f;
    constexpr float kExpC4     = 4.13813679705723846039e-08f;
    constexpr float kLogNat2Lo = 1.90821492927058770002e-10f;

    constexpr float k_invLogNat2 = 1.f / k_ln2;
    const float     kk           = Round(_x * k_invLogNat2);
    const float     hi           = _x - kk * k_ln2;
    const float     lo           = kk * kLogNat2Lo;
    const float     hml          = hi - lo;
    const float     hmlsq        = hml * hml;
    const float     tmp0         = kExpC4 * hmlsq + kExpC3;
    const float     tmp1         = tmp0 * hmlsq + kExpC2;
    const float     tmp2         = tmp1 * hmlsq + kExpC1;
    const float     tmp3         = tmp2 * hmlsq + kExpC0;
    const float     tmp4         = hml - hmlsq * tmp3;
    const float     tmp5         = hml * tmp4 / (2.0f - tmp4);
    const float     tmp6         = 1.0f - ((lo - tmp5) - hi);
    const float     result       = LdExp(tmp6, static_cast<Int32>(kk));

    return result;
}

[[nodiscard]] constexpr float Power(const float _x, const float _y)
{
    if (Abs(_y) < Limits<float>::k_min)
    {
        return 1.f;
    }

    if (Abs(_x) < Limits<float>::k_min)
    {
        return 0.f;
    }

    return CopySign(Exp(_y * Log(Abs(_x))), _x);
}

[[nodiscard]] constexpr float Sqrt(const float _x)
{
    if (std::is_constant_evaluated())
    {
        if (_x < 0.0f)
        {
            return BitsToFloat(k_floatExponentMask | k_floatMantissaMask);
        }

        return _x * Power(_x, -0.5f);
    }
    else
    {
        if (_x < 0.0f)
        {
            return BitsToFloat(k_floatExponentMask | k_floatMantissaMask);
        }
        else if (_x < Limits<float>::k_min)
        {
            return 0.0f;
        }

        const __m128 aa   = _mm_set1_ps(_x);
        const __m128 sqrt = _mm_sqrt_ps(aa);

        float result = 0.0f;
        _mm_store_ss(reinterpret_cast<float*>(&result), sqrt);
        return result;
    }
}

[[nodiscard]] constexpr float InvSqrt(const float _x)
{
    if (std::is_constant_evaluated())
    {
        if (_x < Limits<float>::k_min)
        {
            return k_inf;
        }

        return Power(_x, -0.5f);
    }
    else
    {
        if (_x < Limits<float>::k_min)
        {
            return k_inf;
        }

        const __m128 aa    = _mm_set1_ps(_x);
        const __m128 rsqrt = _mm_rsqrt_ps(aa);

        float result = 0.0f;
        _mm_store_ss(&result, rsqrt);
        return result;
    }
}

[[nodiscard]] constexpr float Cos(const float _x)
{
    const float scaled = _x * 2.0f * k_invPi;
    const float real   = Floor(scaled);
    const float xx     = _x - real * k_halfPi;
    const Int32 bits   = static_cast<Int32>(real) & 3;

    constexpr float kSinC2  = -0.16666667163372039794921875f;
    constexpr float kSinC4  = 8.333347737789154052734375e-3f;
    constexpr float kSinC6  = -1.9842604524455964565277099609375e-4f;
    constexpr float kSinC8  = 2.760012648650445044040679931640625e-6f;
    constexpr float kSinC10 = -2.50293279435709337121807038784027099609375e-8f;

    constexpr float kCosC2  = -0.5f;
    constexpr float kCosC4  = 4.166664183139801025390625e-2f;
    constexpr float kCosC6  = -1.388833043165504932403564453125e-3f;
    constexpr float kCosC8  = 2.47562347794882953166961669921875e-5f;
    constexpr float kCosC10 = -2.59630184018533327616751194000244140625e-7f;

    float c0  = xx;
    float c2  = kSinC2;
    float c4  = kSinC4;
    float c6  = kSinC6;
    float c8  = kSinC8;
    float c10 = kSinC10;

    if (bits == 0
        || bits == 2)
    {
        c0  = 1.0f;
        c2  = kCosC2;
        c4  = kCosC4;
        c6  = kCosC6;
        c8  = kCosC8;
        c10 = kCosC10;
    }

    const float xsq    = xx * xx;
    const float tmp0   = c10 * xsq + c8;
    const float tmp1   = tmp0 * xsq + c6;
    const float tmp2   = tmp1 * xsq + c4;
    const float tmp3   = tmp2 * xsq + c2;
    const float tmp4   = tmp3 * xsq + 1.f;
    const float result = tmp4 * c0;

    return bits == 1 || bits == 2 ? -result : result;
}

[[nodiscard]] constexpr float Sin(const float _x)
{
    return Cos(_x - k_halfPi);
}

[[nodiscard]] constexpr float Tan(const float _x)
{
    return Sin(_x) / Cos(_x);
}

[[nodiscard]] constexpr SinCos SinCosApprox(const float _x)
{
    const float aa     = _x - Floor(_x * k_inv2Pi) * k_2pi;
    const float absA   = Abs(aa);
    const float cosA   = Cos(absA);
    const float cosASq = cosA * cosA;
    const float tmp0   = Sqrt(1.0f - cosASq);
    const float tmp1   = aa > 0.0f && aa < k_pi ? 1.0f : -1.0f;
    const float sinA   = tmp0 * tmp1;
    return { sinA, cosA };
}

[[nodiscard]] constexpr float Acos(const float _a)
{
    constexpr float kAcosC0 = 1.5707288f;
    constexpr float kAcosC1 = -0.2121144f;
    constexpr float kAcosC2 = 0.0742610f;
    constexpr float kAcosC3 = -0.0187293f;

    const float absa   = Abs(_a);
    const float tmp0   = kAcosC3 * absa + kAcosC2;
    const float tmp1   = tmp0 * absa + kAcosC1;
    const float tmp2   = tmp1 * absa + kAcosC0;
    const float tmp3   = tmp2 * Sqrt(1.0f - absa);
    const float negate = static_cast<float>(_a < 0.0f);
    const float tmp4   = tmp3 - 2.0f * negate * tmp3;
    const float result = negate * k_pi + tmp4;

    return result;
}

[[nodiscard]] constexpr float Asin(const float _a)
{
    return k_halfPi - Acos(_a);
}

[[nodiscard]] constexpr float Atan2(const float _y, const float _x)
{
    const float ax     = Abs(_x);
    const float ay     = Abs(_y);
    const float maxaxy = Max(ax, ay);
    const float minaxy = Min(ax, ay);

    if (maxaxy == 0.0f)
    {
        return _y < 0.0f ? -0.0f : 0.0f;
    }

    constexpr float kAtan2C0 = -0.013480470f;
    constexpr float kAtan2C1 = 0.057477314f;
    constexpr float kAtan2C2 = -0.121239071f;
    constexpr float kAtan2C3 = 0.195635925f;
    constexpr float kAtan2C4 = -0.332994597f;
    constexpr float kAtan2C5 = 0.999995630f;

    const float mxy    = minaxy / maxaxy;
    const float mxysq  = mxy * mxy;
    const float tmp0   = kAtan2C0 * mxysq + kAtan2C1;
    const float tmp1   = tmp0 * mxysq + kAtan2C2;
    const float tmp2   = tmp1 * mxysq + kAtan2C3;
    const float tmp3   = tmp2 * mxysq + kAtan2C4;
    const float tmp4   = tmp3 * mxysq + kAtan2C5;
    const float tmp5   = tmp4 * mxy;
    const float tmp6   = ay > ax ? k_halfPi - tmp5 : tmp5;
    const float tmp7   = _x < 0.0f ? k_pi - tmp6 : tmp6;
    const float result = _y < 0.0f ? -tmp7 : tmp7;

    return result;
}

[[nodiscard]] constexpr float ATan(const float _x)
{
    return Atan2(_x, 1.0f);
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
    if (std::is_constant_evaluated())
    {
        return { a.x + b.x, a.y + b.y };
    }
    else
    {
        return DirectX::XMVectorAdd(a, b);
    }
}

[[nodiscard]] constexpr Vec2 Sub(const Vec2 a, const Vec2 b)
{
    if (std::is_constant_evaluated())
    {
        return { a.x - b.x, a.y - b.y };
    }
    else
    {
        return DirectX::XMVectorSubtract(a, b);
    }
}

[[nodiscard]] constexpr Vec3 Add(const Vec3 a, const Vec3 b)
{
    if (std::is_constant_evaluated())
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }
    else
    {
        return DirectX::XMVectorAdd(a, b);
    }
}

[[nodiscard]] constexpr Vec3 Sub(const Vec3 a, const Vec3 b)
{
    if (std::is_constant_evaluated())
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }
    else
    {
        return DirectX::XMVectorSubtract(a, b);
    }
}

[[nodiscard]] constexpr Vec4 Add(const Vec4 a, const Vec4 b)
{
    if (std::is_constant_evaluated())
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }
    else
    {
        return DirectX::XMVectorAdd(a, b);
    }
}

[[nodiscard]] constexpr Vec4 Sub(const Vec4 a, const Vec4 b)
{
    if (std::is_constant_evaluated())
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    }
    else
    {
        return DirectX::XMVectorSubtract(a, b);
    }
}

[[nodiscard]] constexpr float Dot(const Vec2 a, const Vec2 b)
{
    if (std::is_constant_evaluated())
    {
        return a.x * b.x + a.y * b.y;
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector2Dot(a, b));
    }
}

[[nodiscard]] constexpr float Dot(const Vec3 a, const Vec3 b)
{
    if (std::is_constant_evaluated())
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector3Dot(a, b));
    }
}

[[nodiscard]] constexpr float Dot(const Vec4 a, const Vec4 b)
{
    if (std::is_constant_evaluated())
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector4Dot(a, b));
    }
}

[[nodiscard]] constexpr Vec3 Cross(const Vec3 a, const Vec3 b)
{
    if (std::is_constant_evaluated())
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
    else
    {
        return DirectX::XMVector3Cross(a, b);
    }
}

[[nodiscard]] constexpr float LengthSq(const Vec2 vec)
{
    if (std::is_constant_evaluated())
    {
        return vec.x * vec.x + vec.y * vec.y;
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector2LengthSq(vec));
    }
}

[[nodiscard]] constexpr float LengthSq(const Vec3 vec)
{
    if (std::is_constant_evaluated())
    {
        return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vec));
    }
}

[[nodiscard]] constexpr float LengthSq(const Vec4 vec)
{
    if (std::is_constant_evaluated())
    {
        return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w;
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(vec));
    }
}

[[nodiscard]] constexpr float Length(const Vec2 vec)
{
    if (std::is_constant_evaluated())
    {
        return Sqrt(LengthSq(vec));
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector2Length(vec));
    }
}

[[nodiscard]] constexpr float Length(const Vec3 vec)
{
    if (std::is_constant_evaluated())
    {
        return Sqrt(LengthSq(vec));
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector3Length(vec));
    }
}

[[nodiscard]] constexpr float Length(const Vec4 vec)
{
    if (std::is_constant_evaluated())
    {
        return Sqrt(LengthSq(vec));
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector4Length(vec));
    }
}

[[nodiscard]] constexpr float InvLength(const Vec2 vec)
{
    if (std::is_constant_evaluated())
    {
        return InvSqrt(LengthSq(vec));
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector2ReciprocalLength(vec));
    }
}

[[nodiscard]] constexpr float InvLength(const Vec3 vec)
{
    if (std::is_constant_evaluated())
    {
        return InvSqrt(LengthSq(vec));
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector3ReciprocalLength(vec));
    }
}

[[nodiscard]] constexpr float InvLength(const Vec4 vec)
{
    if (std::is_constant_evaluated())
    {
        return InvSqrt(LengthSq(vec));
    }
    else
    {
        return DirectX::XMVectorGetX(DirectX::XMVector4ReciprocalLength(vec));
    }
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

[[nodiscard]] constexpr float Distance(const Vec2 a, const Vec2 b)
{
    return Length(Sub(b, a));
}

[[nodiscard]] constexpr float Distance(const Vec3 a, const Vec3 b)
{
    return Length(Sub(b, a));
}

[[nodiscard]] constexpr float Distance(const Vec4 a, const Vec4 b)
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

[[nodiscard]] constexpr Vec2 Normalized(const Vec2 vec)
{
    if (std::is_constant_evaluated())
    {
        const float lenSq = LengthSq(vec);
        if (IsZeroApprox(lenSq))
        {
            return vec2::k_zero;
        }
        return vec * InvSqrt(lenSq);
    }
    else
    {
        return DirectX::XMVector2Normalize(vec);
    }
}
[[nodiscard]] constexpr Vec3 Normalized(const Vec3 vec)
{
    if (std::is_constant_evaluated())
    {
        const float lenSq = LengthSq(vec);
        if (IsZeroApprox(lenSq))
        {
            return vec3::k_zero;
        }
        return vec * InvSqrt(lenSq);
    }
    else
    {
        return DirectX::XMVector3Normalize(vec);
    }
}

[[nodiscard]] constexpr Vec4 Normalized(const Vec4 vec)
{
    if (std::is_constant_evaluated())
    {
        const float lenSq = LengthSq(vec);
        if (IsZeroApprox(lenSq))
        {
            return vec4::k_zero;
        }
        return vec * InvSqrt(lenSq);
    }
    else
    {
        return DirectX::XMVector4Normalize(vec);
    }
}

[[nodiscard]] constexpr bool IsZeroApprox(const Vec2 vec)
{
    return IsZeroApprox(vec.x) && IsZeroApprox(vec.y);
}

[[nodiscard]] constexpr bool IsZeroApprox(const Vec3 vec)
{
    return IsZeroApprox(vec.x) && IsZeroApprox(vec.y) && IsZeroApprox(vec.z);
}

[[nodiscard]] constexpr bool IsZeroApprox(const Vec4 vec)
{
    return IsZeroApprox(vec.x) && IsZeroApprox(vec.y) && IsZeroApprox(vec.z) && IsZeroApprox(vec.w);
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
        static_assert(AlwaysFalseV<T>, "MakeRandom: T must be an integral or floating-point type.");
        return T {};
    }
}

}   // namespace Jam
