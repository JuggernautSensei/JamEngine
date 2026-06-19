#pragma once

namespace Jam
{

constexpr static float kPi        = 3.1415926535897932f;
constexpr static float k2Pi       = 2.f * kPi;
constexpr static float kHalfPi    = kPi / 2.f;
constexpr static float kInvPi     = 1.f / kPi;
constexpr static float kInv2Pi    = 1.f / k2Pi;
constexpr static float kInvHalfPi = 1.f / kHalfPi;
constexpr static float kE         = 2.7182818284590451f;
constexpr static float kEpsilon   = 1e-6f;
constexpr static float kLn2       = 0.6931471805599453f;
constexpr static float kInvLn2    = 1.f / kLn2;
constexpr static float kLn10      = 2.302585092994046f;
constexpr static float kSqrt2     = 1.4142135623730951f;
constexpr static float kToRad     = kPi / 180.f;
constexpr static float kToDeg     = 180.f / kPi;

template<typename T>
struct Limits
{
};

template<>
struct Limits<bool>
{
    static constexpr bool kMin     = false;
    static constexpr bool kMax     = true;
    static constexpr bool kHighest = true;
    static constexpr bool kLowest  = false;
};

template<>
struct Limits<double>
{
    static constexpr double kMin     = DBL_MIN;
    static constexpr double kMax     = DBL_MAX;
    static constexpr double kHighest = DBL_MAX;
    static constexpr double kLowest  = -DBL_MAX;
};

template<>
struct Limits<float>
{
    static constexpr float kMin     = FLT_MIN;
    static constexpr float kMax     = FLT_MAX;
    static constexpr float kHighest = FLT_MAX;
    static constexpr float kLowest  = -FLT_MAX;
};

template<>
struct Limits<Int32>
{
    static constexpr Int32 kMin     = INT32_MIN;
    static constexpr Int32 kMax     = INT32_MAX;
    static constexpr Int32 kHighest = INT32_MAX;
    static constexpr Int32 kLowest  = INT32_MIN;
};

template<>
struct Limits<UInt32>
{
    static constexpr UInt32 kMin     = 0;
    static constexpr UInt32 kMax     = UINT32_MAX;
    static constexpr UInt32 kHighest = UINT32_MAX;
    static constexpr UInt32 kLowest  = 0;
};

template<>
struct Limits<Int64>
{
    static constexpr Int64 kMin     = INT64_MIN;
    static constexpr Int64 kMax     = INT64_MAX;
    static constexpr Int64 kHighest = INT64_MAX;
    static constexpr Int64 kLowest  = INT64_MIN;
};

template<>
struct Limits<UInt64>
{
    static constexpr UInt64 kMin     = 0;
    static constexpr UInt64 kMax     = UINT64_MAX;
    static constexpr UInt64 kHighest = UINT64_MAX;
    static constexpr UInt64 kLowest  = 0;
};

template<>
struct Limits<UInt16>
{
    static constexpr UInt16 kMin     = 0;
    static constexpr UInt16 kMax     = UINT16_MAX;
    static constexpr UInt16 kHighest = UINT16_MAX;
    static constexpr UInt16 kLowest  = 0;
};

template<>
struct Limits<Int16>
{
    static constexpr Int16 kMin     = INT16_MIN;
    static constexpr Int16 kMax     = INT16_MAX;
    static constexpr Int16 kHighest = INT16_MAX;
    static constexpr Int16 kLowest  = INT16_MIN;
};

template<>
struct Limits<UInt8>
{
    static constexpr UInt8 kMin     = 0;
    static constexpr UInt8 kMax     = UINT8_MAX;
    static constexpr UInt8 kHighest = UINT8_MAX;
    static constexpr UInt8 kLowest  = 0;
};

template<>
struct Limits<Int8>
{
    static constexpr Int8 kMin     = INT8_MIN;
    static constexpr Int8 kMax     = INT8_MAX;
    static constexpr Int8 kHighest = INT8_MAX;
    static constexpr Int8 kLowest  = INT8_MIN;
};

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
    return degrees * kToRad;
}
[[nodiscard]] constexpr float ToDeg(const float radians)
{
    return radians * kToDeg;
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
    return -0.0f == _x ? 0.0f != _x : 0.0f > _x;   // NOLINT(clang-diagnostic-float-equal)
}

[[nodiscard]] constexpr float CopySign(const float _value, const float _sign)
{
    return SignBit(_value) != SignBit(_sign) ? -_value : _value;
}

[[nodiscard]] constexpr float Mod(const float _x, const float _y)
{
    return _x - _y * Floor(_x / _y);
}

[[nodiscard]] constexpr bool IsZeroApprox(const float _x)
{
    return Abs(_x) < kEpsilon;
}

[[nodiscard]] constexpr bool IsEqualApprox(const float _x, const float _y)
{
    return Abs(_x - _y) < kEpsilon;
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
    return std::log2f(_x);
}

[[nodiscard]] inline float LdExp(const float _a, const Int32 _b)
{
    return std::ldexpf(_a, _b);
}

[[nodiscard]] inline float Exp(const float _x)
{
    return std::expf(_x);
}

[[nodiscard]] inline float Pow(const float _x, const float _y)
{
    return std::powf(_x, _y);
}

[[nodiscard]] inline float Sqrt(const float _x)
{
    return std::sqrtf(_x);
}

[[nodiscard]] inline float RSqrt(const float _x)
{
    return 1.f / std::sqrtf(_x);
}

[[nodiscard]] inline float Sin(const float _x)
{
    return std::sinf(_x);
}

[[nodiscard]] inline float Cos(const float _x)
{
    return std::cosf(_x);
}

[[nodiscard]] inline float Tan(const float _x)
{
    return std::tanf(_x);
}

[[nodiscard]] inline float ATan(const float _x)
{
    return std::atanf(_x);
}

[[nodiscard]] inline float Asin(const float _a)
{
    return std::asinf(_a);
}

[[nodiscard]] inline float Acos(const float _a)
{
    return std::acosf(_a);
}

[[nodiscard]] inline float Atan2(const float _y, const float _x)
{
    return std::atan2f(_y, _x);
}

}   // namespace Jam