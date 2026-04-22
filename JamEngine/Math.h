#pragma once
#include "MathConstant.h"

namespace Jam
{

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

[[nodiscard]] constexpr bool IsZeroApprox(const float _x, const float _epsilon = kEpsilon)
{
    return Abs(_x) < _epsilon;
}

[[nodiscard]] constexpr bool IsEqualApprox(const float _x, const float _y, const float _epsilon = kEpsilon)
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

[[nodiscard]] inline float Power(const float _x, const float _y)
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