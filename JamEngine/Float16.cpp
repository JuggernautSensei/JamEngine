// ReSharper disable CppClangTidyClangDiagnosticImplicitIntConversion
#include "pch.h"
#include "Float16.h"

namespace Jam
{

Float16 PackFloat32to16(const float _value)
{
    union
    {
        float  fv;
        UInt32 ui;
    } ci;
    ci.fv = _value;

    const UInt32 x        = ci.ui;
    const UInt32 sign     = x >> 31;
    const UInt32 exponent = (x >> 23) & 0xFF;
    const UInt32 mantissa = x & 0x7FFFFF;

    UInt16 hf = 0;

    if (exponent == 0xFF)
    {
        hf = (static_cast<UInt16>(sign) << 15)
           | 0x7C00
           | (mantissa >> 13);
    }

    else if (exponent > 0x8E)
    {
        hf = (static_cast<UInt16>(sign) << 15)
           | 0x7C00;
    }

    else if (exponent > 0x70)
    {
        hf = (static_cast<UInt16>(sign) << 15)
           | (static_cast<UInt16>((exponent - 112) << 10))
           | (mantissa >> 13);
    }

    else if (exponent > 0x66)
    {
        UInt32 man = (0x800000 | mantissa);

        UInt32 shift = 125 - exponent;
        man >>= shift;

        UInt32 rounding_bits = (0x800000 | mantissa) & ((1u << shift) - 1);
        if (rounding_bits > (1u << (shift - 1)))
        {
            man++;
        }
        else if (rounding_bits == (1u << (shift - 1)))
        {
            man = (man + 1) & ~1u;
        }

        hf = (static_cast<UInt16>(sign) << 15) | static_cast<UInt16>(man);
    }

    else
    {
        hf = static_cast<UInt16>(sign << 15);
    }

    return static_cast<Float16>(hf);
}

float UnpackFloat16To32(const Float16 _value)
{
    union
    {
        float  fv;
        UInt32 ui;
    } co;

    const UInt16 hf       = static_cast<UInt16>(_value);
    const UInt16 sign     = (hf >> 15) & 0x1;
    const UInt16 exponent = (hf >> 10) & 0x1F;
    const UInt16 mantissa = hf & 0x3FF;

    if (exponent == 0x1F)
    {
        co.ui = (sign << 31) | 0x7F800000 | (mantissa << 13);
    }
    else if (exponent != 0)
    {
        co.ui = (sign << 31) | ((exponent + 112) << 23) | (mantissa << 13);
    }
    else
    {
        co.ui = (sign << 31) | (mantissa << 13);
    }

    return co.fv;
}

}   // namespace Jam