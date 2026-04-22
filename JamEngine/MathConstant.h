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

}   // namespace Jam