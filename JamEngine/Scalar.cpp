#include "pch.h"
#include "Scalar.h"

namespace Jam
{

Scalar HighestValueOf(
    const eScalarType _type)
{
    return MaxValueOf(_type);
}

Scalar LowestValueOf(const eScalarType _type)
{
    switch (_type)
    {
        case eScalarType::Bool: return Limits<bool>::k_lowest;
        case eScalarType::Int8: return Limits<Int8>::k_lowest;
        case eScalarType::UInt8: return Limits<UInt8>::k_lowest;
        case eScalarType::Int16: return Limits<Int16>::k_lowest;
        case eScalarType::UInt16: return Limits<UInt16>::k_lowest;
        case eScalarType::Int32: return Limits<Int32>::k_lowest;
        case eScalarType::UInt32: return Limits<UInt32>::k_lowest;
        case eScalarType::Int64: return Limits<Int64>::k_lowest;
        case eScalarType::UInt64: return Limits<UInt64>::k_lowest;
        case eScalarType::Float: return Limits<float>::k_lowest;
        default:
            JAM_UNREACHABLE;
    }
}

Scalar MinValueOf(const eScalarType _type)
{
    switch (_type)
    {
        case eScalarType::Bool: return Limits<bool>::k_min;
        case eScalarType::Int8: return Limits<Int8>::k_min;
        case eScalarType::UInt8: return Limits<UInt8>::k_min;
        case eScalarType::Int16: return Limits<Int16>::k_min;
        case eScalarType::UInt16: return Limits<UInt16>::k_min;
        case eScalarType::Int32: return Limits<Int32>::k_min;
        case eScalarType::UInt32: return Limits<UInt32>::k_min;
        case eScalarType::Int64: return Limits<Int64>::k_min;
        case eScalarType::UInt64: return Limits<UInt64>::k_min;
        case eScalarType::Float: return Limits<float>::k_min;
        default:
            JAM_UNREACHABLE;
    }
}

Scalar MaxValueOf(const eScalarType _type)
{
    switch (_type)
    {
        case eScalarType::Bool: return Limits<bool>::k_max;
        case eScalarType::Int8: return Limits<Int8>::k_max;
        case eScalarType::UInt8: return Limits<UInt8>::k_max;
        case eScalarType::Int16: return Limits<Int16>::k_max;
        case eScalarType::UInt16: return Limits<UInt16>::k_max;
        case eScalarType::Int32: return Limits<Int32>::k_max;
        case eScalarType::UInt32: return Limits<UInt32>::k_max;
        case eScalarType::Int64: return Limits<Int64>::k_max;
        case eScalarType::UInt64: return Limits<UInt64>::k_max;
        case eScalarType::Float: return Limits<float>::k_max;
        default:
            JAM_UNREACHABLE;
    }
}

Int32 ByteOf(const eScalarType _type)
{
    switch (_type)
    {
        case eScalarType::Bool:
            return sizeof(bool);
        case eScalarType::Int8:
            return sizeof(Int8);
        case eScalarType::UInt8:
            return sizeof(UInt8);
        case eScalarType::Int16:
            return sizeof(Int16);
        case eScalarType::UInt16:
            return sizeof(UInt16);
        case eScalarType::Int32:
            return sizeof(Int32);
        case eScalarType::UInt32:
            return sizeof(UInt32);
        case eScalarType::Int64:
            return sizeof(Int64);
        case eScalarType::UInt64:
            return sizeof(UInt64);
        case eScalarType::Float:
            return sizeof(float);
        default:
            JAM_UNREACHABLE;
    }
}

}   // namespace Jam