#pragma once

namespace Jam
{

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr T AlignUp(const T _value, const T _alignment) noexcept
{
    JAM_ASSERT(_alignment > 0 && (_alignment & (_alignment - 1)) == 0);   // alignment must be power of 2
    return (_value + (_alignment - 1)) & ~(_alignment - 1);
}

template<typename T>
[[nodiscard]] constexpr bool MemEqual(const T& _lhs, const T& _rhs) noexcept
{
    return std::memcmp(&_lhs, &_rhs, sizeof(T)) == 0;
}

template<typename T, typename R>
    requires std::is_arithmetic_v<T> && std::is_arithmetic_v<R>
[[nodiscard]] constexpr bool CmpLess(const T _a, const R _b) noexcept
{
    using CommonT = std::common_type_t<T, R>;

    if constexpr (std::is_signed_v<T> == std::is_signed_v<R>
                  || (std::is_floating_point_v<T> || std::is_floating_point_v<R>))
    {
        return static_cast<CommonT>(_a) < static_cast<CommonT>(_b);
    }
    else
    {
        if constexpr (std::is_signed_v<T>)
        {
            // _a is signed
            return _a < 0 || static_cast<CommonT>(_a) < static_cast<CommonT>(_b);
        }
        else
        {
            // _b is signed
            return _b >= 0 && static_cast<CommonT>(_a) < static_cast<CommonT>(_b);
        }
    }
}

template<typename T, typename R>
    requires std::is_arithmetic_v<T> && std::is_arithmetic_v<R>
[[nodiscard]] constexpr bool CmpGreater(const T _a, const R _b) noexcept
{
    return CmpLess(_b, _a);
}

template<typename T, typename R>
    requires std::is_arithmetic_v<T> && std::is_arithmetic_v<R>
[[nodiscard]] constexpr bool CmpLessEqual(const T& _a, const R& _b) noexcept
{
    return !CmpGreater(_a, _b);
}

template<typename T, typename R>
    requires std::is_arithmetic_v<T> && std::is_arithmetic_v<R>
[[nodiscard]] constexpr bool CmpGreaterEqual(const T& _a, const R& _b) noexcept
{
    return !CmpLess(_a, _b);
}

template<typename T, typename R>
    requires std::is_integral_v<T> && std::is_integral_v<R>
[[nodiscard]] constexpr bool CmpEqual(const T _a, const R _b) noexcept   // can not use floating_pointer_t
{
    using CommonT = std::common_type_t<T, R>;
    if constexpr (std::is_signed_v<T> == std::is_signed_v<R>)
    {
        return static_cast<CommonT>(_a) == static_cast<CommonT>(_b);
    }
    else if constexpr (std::is_signed_v<T>)
    {
        return _a >= 0 && static_cast<CommonT>(_a) == static_cast<CommonT>(_b);
    }
    else
    {
        return _b >= 0 && static_cast<CommonT>(_a) == static_cast<CommonT>(_b);
    }
}

template<typename T, typename R>
    requires std::is_integral_v<T> && std::is_integral_v<R>
[[nodiscard]] constexpr bool CmpNotEqual(const T _a, const R _b) noexcept
{
    return !CmpEqual(_a, _b);
}

template<typename T, USizeT N>
[[nodiscard]] constexpr SizeT CountOf(const T (&)[N]) noexcept
{
    return N;
}

}   // namespace Jam