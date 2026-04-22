#pragma once

namespace Jam
{

[[nodiscard]] constexpr Int32 UInt32CountOne(
    const UInt32 _value)
{
    const UInt32 tmp0   = (_value >> 1) & 0x55555555;
    const UInt32 tmp1   = _value - tmp0;
    const UInt32 tmp2   = tmp1 & 0xc30c30c3;
    const UInt32 tmp3   = (tmp1 >> 2) & 0xc30c30c3;
    const UInt32 tmp4   = (tmp1 >> 4) & 0xc30c30c3;
    const UInt32 tmp5   = tmp2 + tmp3;
    const UInt32 tmp6   = tmp4 + tmp5;
    const UInt32 tmp7   = tmp6 + (tmp6 >> 6);
    const UInt32 tmp8   = tmp7 + (tmp7 >> 12);
    const UInt32 tmp9   = tmp8 + (tmp8 >> 24);
    const UInt32 result = tmp9 & 0x3f;
    return static_cast<Int32>(result);
}

[[nodiscard]] constexpr Int32 UInt32CountLZero(
    const UInt32 _value)
{
    UInt32 val = _value;
    val |= (val >> 1);
    val |= (val >> 2);
    val |= (val >> 4);
    val |= (val >> 8);
    val |= (val >> 16);
    return UInt32CountOne(~val);
}

[[nodiscard]] constexpr Int32 UInt32CountRZero(
    const UInt32 _value)
{
    const UInt32 tmp0 = ~_value;
    const UInt32 tmp1 = _value - 1;
    const UInt32 tmp2 = tmp0 & tmp1;
    return UInt32CountOne(tmp2);
}

[[nodiscard]] constexpr Int32 UInt64CountOne(
    const UInt64 _value)
{
    const UInt32 low  = static_cast<UInt32>(_value & 0xFFFF'FFFF);
    const UInt32 high = static_cast<UInt32>(_value >> 32);
    return UInt32CountOne(low) + UInt32CountOne(high);
}

[[nodiscard]] constexpr Int32 UInt64CountLZero(
    const UInt64 _value)
{
    return (_value & 0xFFFF'FFFF'0000'0000) ? UInt32CountLZero(static_cast<UInt32>(_value >> 32)) : UInt32CountLZero(static_cast<UInt32>(_value)) + 32;
}

[[nodiscard]] constexpr Int32 UInt64CountRZero(
    const UInt64 _value)
{
    return (_value & 0x0000'0000'FFFF'FFFF) ? UInt32CountRZero(static_cast<UInt32>(_value)) : UInt32CountRZero(static_cast<UInt32>(_value >> 32)) + 32;
}

}   // namespace Jam