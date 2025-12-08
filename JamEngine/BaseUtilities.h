#pragma once
#include "MathUtilities.h"

namespace jam
{

#ifndef JAM_NEW
#    define JAM_NEW(_type, ...) new _type(__VA_ARGS__)
#endif

#ifndef JAM_DELETE
#    define JAM_DELETE(_ptr)  \
        JAM_BEGIN_MACRO_BLOCK \
        delete (_ptr);        \
        (_ptr) = nullptr;     \
        JAM_END_MACRO_BLOCK
#endif

#ifndef JAM_DELETE_ARRAY
#    define JAM_DELETE_ARRAY(_ptr) \
        JAM_BEGIN_MACRO_BLOCK      \
        delete[] (_ptr);           \
        (_ptr) = nullptr;          \
        JAM_END_MACRO_BLOCK
#endif

template<typename To, typename From>
    requires std::is_convertible_v<From, To>
constexpr FORCEINLINE To NarrowCast(From value) noexcept
{
    const To to = static_cast<To>(value);
    JAM_ASSERT(static_cast<From>(to) == value, "NarrowCast failed: expected {}, got {}", value, to);
    return to;
}

template<typename T>
    requires std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>
[[nodiscard]] bool MemEqual(const T& _lhs, const T& _rhs) noexcept
{
    return std::memcmp(&_lhs, &_rhs, sizeof(T)) == 0;
}

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr T SetBit(T _value, T _bits, const bool _bEnable)
{
    if (_bEnable)
    {
        return _value | _bits;
    }
    else
    {
        return _value & ~_bits;
    }
}

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr T EnableBit(T _value, T _bits)
{
    return _value | _bits;
}

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr T DisableBit(T _value, T _bits)
{
    return _value & ~_bits;
}

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr T ToggleBit(T _value, T _bits)
{
    return _value ^ _bits;
}

}   // namespace jam