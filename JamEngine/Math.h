#pragma once
#include <numbers>
#include <random>

namespace jam
{

constexpr float k_pi        = std::numbers::pi_v<float>;
constexpr float k_2pi       = 2.f * k_pi;
constexpr float k_halfPi    = k_pi / 2.f;
constexpr float k_invPi     = 1.f / k_pi;
constexpr float k_inv2Pi    = 1.f / k_2pi;
constexpr float k_invHalfPi = 1.f / k_halfPi;
constexpr float k_epsilon   = 1e-5f;
constexpr float k_toRad     = k_pi / 180.f;
constexpr float k_toDeg     = 180.f / k_pi;

constexpr float  k_floatMin  = std::numeric_limits<float>::lowest();
constexpr float  k_floatMax  = std::numeric_limits<float>::max();
constexpr UInt64 k_uint64Min = std::numeric_limits<UInt64>::min();
constexpr UInt64 k_uint64Max = std::numeric_limits<UInt64>::max();
constexpr Int64  k_int64Min  = std::numeric_limits<Int64>::min();
constexpr Int64  k_int64Max  = std::numeric_limits<Int64>::max();
constexpr UInt32 k_uint32Min = std::numeric_limits<UInt32>::min();
constexpr UInt32 k_uint32Max = std::numeric_limits<UInt32>::max();
constexpr Int32  k_int32Min  = std::numeric_limits<Int32>::min();
constexpr Int32  k_int32Max  = std::numeric_limits<Int32>::max();
constexpr UInt16 k_uint16Min = std::numeric_limits<UInt16>::min();
constexpr UInt16 k_uint16Max = std::numeric_limits<UInt16>::max();
constexpr Int16  k_int16Min  = std::numeric_limits<Int16>::min();
constexpr Int16  k_int16Max  = std::numeric_limits<Int16>::max();
constexpr UInt8  k_uint8Min  = std::numeric_limits<UInt8>::min();
constexpr UInt8  k_uint8Max  = std::numeric_limits<UInt8>::max();
constexpr Int8   k_int8Min   = std::numeric_limits<Int8>::min();
constexpr Int8   k_int8Max   = std::numeric_limits<Int8>::max();

constexpr UInt32 k_floatExponentMask     = UINT32_C(0x7f800000);
constexpr UInt32 k_floatMantissaMask     = UINT32_C(0x007fffff);
constexpr UInt32 k_floatSignMask         = UINT32_C(0x80000000);
constexpr UInt32 k_floatExponentBitShift = 23;
constexpr float  k_floatInfinity         = std::numeric_limits<float>::infinity();
constexpr float  k_floatSmallest         = 1.175494351e-38f;
constexpr float  k_floatLargest          = 3.402823466e+38f;
constexpr float  k_sqrt2                 = std::numbers::sqrt2_v<float>;
constexpr float  k_logNat10              = std::numbers::ln10_v<float>;
constexpr float  k_logNat2               = std::numbers::ln2_v<float>;
constexpr float  k_invLogNat2            = std::numbers::log2e_v<float>;
constexpr float  k_e                     = std::numbers::e_v<float>;

struct Vec2i
{
    Int32 x = 0;
    Int32 y = 0;
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
    return degrees * (k_pi / 180.f);
}
[[nodiscard]] constexpr float ToDeg(const float radians)
{
    return radians * (180.f / k_pi);
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
    return static_cast<float>(static_cast<int>(_x));
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

[[nodiscard]] constexpr bool IsZeroApprox(const float _x)
{
    return Abs(_x) < k_epsilon;
}

[[nodiscard]] constexpr bool IsEqualApprox(const float _x, const float _y)
{
    return Abs(_x - _y) < k_epsilon;
}

template<typename T>
[[nodiscard]] __forceinline T Div(const T num, const T denom)
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
        static_assert(AlwaysFalse<T>, "Div only supports integral and floating point types");
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
        return -k_floatInfinity;
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
    const float kk = static_cast<float>(exp);
    const float hi = kk * k_logNat2;
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
    return Log(_x) * k_invLogNat2;
}

[[nodiscard]] constexpr UInt32 Log2(const UInt32 _x)
{
    constexpr Int32 k_log2Table[32] = {
        0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
    };

    UInt32 result = _x;
    result |= result >> 1;
    result |= result >> 2;
    result |= result >> 4;
    result |= result >> 8;
    result |= result >> 16;
    return k_log2Table[static_cast<UInt32>(result * 0x07C4ACDD) >> 27];
}

[[nodiscard]] constexpr Int32 Log2(const Int32 _x)
{
    return static_cast<Int32>(Log2(static_cast<UInt32>(_x)));
}

[[nodiscard]] constexpr UInt64 Log2(const UInt64 _x)
{
    constexpr Int32 k_log2Table[64] = {
        63, 0, 58, 1, 59, 47, 53, 2, 60, 39, 48, 27, 54, 33, 42, 3, 61, 51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4, 62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56, 45, 25, 31, 35, 16, 9, 12, 44, 24, 15, 8, 23, 7, 6, 5
    };

    UInt64 result = _x;
    result |= result >> 1;
    result |= result >> 2;
    result |= result >> 4;
    result |= result >> 8;
    result |= result >> 16;
    result |= result >> 32;
    return k_log2Table[static_cast<UInt64>((result - (result >> 1)) * 0x07EDD5E59A4E28C2) >> 58];
}

[[nodiscard]] constexpr Int64 Log2(const Int64 _x)
{
    return static_cast<Int64>(Log2(static_cast<UInt64>(_x)));
}

[[nodiscard]] constexpr bool IsInfinite(const float _x)
{
    const UInt32 tmp = FloatToBits(_x) & INT32_MAX;
    return tmp == k_floatExponentMask;
}

[[nodiscard]] constexpr float LdExp(const float _a, const int32_t _b)
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

    constexpr float expMin = Log(k_floatSmallest);

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

    const float kk     = Round(_x * k_invLogNat2);
    const float hi     = _x - kk * k_logNat2;
    const float lo     = kk * kLogNat2Lo;
    const float hml    = hi - lo;
    const float hmlsq  = hml * hml;
    const float tmp0   = kExpC4 * hmlsq + kExpC3;
    const float tmp1   = tmp0 * hmlsq + kExpC2;
    const float tmp2   = tmp1 * hmlsq + kExpC1;
    const float tmp3   = tmp2 * hmlsq + kExpC0;
    const float tmp4   = hml - hmlsq * tmp3;
    const float tmp5   = hml * tmp4 / (2.0f - tmp4);
    const float tmp6   = 1.0f - ((lo - tmp5) - hi);
    const float result = LdExp(tmp6, static_cast<Int32>(kk));

    return result;
}

[[nodiscard]] constexpr float Power(const float _x, const float _y)
{
    if (Abs(_y) < k_floatSmallest)
    {
        return 1.f;
    }

    if (Abs(_x) < k_floatSmallest)
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
        else if (_x < k_floatSmallest)
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
        if (_x < k_floatSmallest)
        {
            return k_floatInfinity;
        }

        return Power(_x, -0.5f);
    }
    else
    {
        if (_x < k_floatSmallest)
        {
            return k_floatInfinity;
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
    const float   scaled = _x * 2.0f * k_invPi;
    const float   real   = Floor(scaled);
    const float   xx     = _x - real * k_halfPi;
    const int32_t bits   = static_cast<int32_t>(real) & 3;

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

struct SinCos
{
    float sin = 0.f;
    float cos = 0.f;
};

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
    return LengthSq(b - a);
}

[[nodiscard]] constexpr float DistanceSq(const Vec3 a, const Vec3 b)
{
    return LengthSq(b - a);
}

[[nodiscard]] constexpr float DistanceSq(const Vec4 a, const Vec4 b)
{
    return LengthSq(b - a);
}

[[nodiscard]] constexpr float Distance(const Vec2 a, const Vec2 b)
{
    return Length(b - a);
}

[[nodiscard]] constexpr float Distance(const Vec3 a, const Vec3 b)
{
    return Length(b - a);
}

[[nodiscard]] constexpr float Distance(const Vec4 a, const Vec4 b)
{
    return Length(b - a);
}

[[nodiscard]] constexpr Vec2 ToVec2(Vec3 a)
{
    return { a.x, a.y };
}

[[nodiscard]] constexpr Vec2 ToVec2(Vec4 a)
{
    return { a.x, a.y };
}

[[nodiscard]] constexpr Vec3 ToVec3(Vec4 a)
{
    return { a.x, a.y, a.z };
}

[[nodiscard]] constexpr Vec4 ToVec4(Vec3 a, float w = 0.f)
{
    return { a.x, a.y, a.z, w };
}

[[nodiscard]] constexpr Vec4 ToVec4(Vec2 a, float z = 0.f, float w = 0.f)
{
    return { a.x, a.y, z, w };
}

[[nodiscard]] constexpr Vec2 Normalized(const Vec2 vec)
{
    if (std::is_constant_evaluated())
    {
        const float lenSq = LengthSq(vec);
        if (IsZeroApprox(lenSq))
        {
            return Vec2::Zero;
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
            return Vec3::Zero;
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
            return Vec4::Zero;
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
    return IsZeroApprox(LengthSq(vec));
}

[[nodiscard]] constexpr bool IsZeroApprox(const Vec3 vec)
{
    return IsZeroApprox(LengthSq(vec));
}

[[nodiscard]] constexpr bool IsZeroApprox(const Vec4 vec)
{
    return IsZeroApprox(LengthSq(vec));
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

[[nodiscard]] Vec2 ForwardToPitchYaw(Vec3 _forward);
[[nodiscard]] Vec3 PitchYawToForward(float pitch, float yaw);

[[nodiscard]] Mat4 CreateOrthoMatrix(float width, float height, float nearZ, float farZ);
[[nodiscard]] Mat4 CreateOrthoMatrix(float left, float right, float bottom, float top, float nearZ, float farZ);
[[nodiscard]] Mat4 CreatePerspectiveMatrix(float fovYRad, float aspect, float zNear, float zFar);

namespace details
{
    extern std::mt19937 g_randomGenerator;
    extern UInt32       g_randomSeed;
}   // namespace details

template<typename T>
[[nodiscard]] T MakeRandom(T _min, T _max, const UInt32 _seed = std::mt19937::default_seed)
{
    if (_seed != details::g_randomSeed)
    {
        details::g_randomGenerator.seed(_seed);
        details::g_randomSeed = _seed;
    }

    if constexpr (std::is_integral_v<T>)
    {
        std::uniform_int_distribution<T> dist(_min, _max);
        return dist(details::g_randomGenerator);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        std::uniform_real_distribution<T> dist(_min, _max);
        return dist(details::g_randomGenerator);
    }
    else
    {
        static_assert(AlwaysFalse, "MakeRandom: T must be an integral or floating-point type.");
        return T {};
    }
}

}   // namespace jam
