#pragma once

namespace Jam
{

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr T AlignUp(const T _value, const T _alignment) noexcept
{
    JAM_ASSERT(_alignment > 0 && (_alignment & (_alignment - 1)) == 0, "Alignment must be a power of two. Given alignment: {}", _alignment);
    return (_value + (_alignment - 1)) & ~(_alignment - 1);
}

template<typename T, USizeT N>
[[nodiscard]] constexpr SizeT CountOf(const T (&)[N]) noexcept
{
    return N;
}

template<typename T, typename U>
    requires std::is_integral_v<T> && std::is_integral_v<U>
[[nodiscard]] constexpr bool Less(T _lhs, U _rhs) noexcept
{
    if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>)
    {
        return _lhs < 0 ? true : static_cast<std::make_unsigned_t<T>>(_lhs) < _rhs;
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U>)
    {
        return _rhs < 0 ? false : _lhs < static_cast<std::make_unsigned_t<U>>(_rhs);
    }
    else
    {
        return _lhs < _rhs;
    }
}

template<typename T, typename U>
    requires std::is_integral_v<T> && std::is_integral_v<U>
[[nodiscard]] constexpr bool Greater(T _lhs, U _rhs) noexcept
{
    if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>)
    {
        return _lhs < 0 ? false : static_cast<std::make_unsigned_t<T>>(_lhs) > _rhs;
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U>)
    {
        return _rhs < 0 ? true : _lhs > static_cast<std::make_unsigned_t<U>>(_rhs);
    }
    else
    {
        return _lhs > _rhs;
    }
}

template<typename T, typename U>
    requires std::is_integral_v<T> && std::is_integral_v<U>
[[nodiscard]] constexpr bool LessEqual(T _lhs, U _rhs) noexcept
{
    if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>)
    {
        return _lhs < 0 ? true : static_cast<std::make_unsigned_t<T>>(_lhs) <= _rhs;
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U>)
    {
        return _rhs < 0 ? false : _lhs <= static_cast<std::make_unsigned_t<U>>(_rhs);
    }
    else
    {
        return _lhs <= _rhs;
    }
}

template<typename T, typename U>
    requires std::is_integral_v<T> && std::is_integral_v<U>
[[nodiscard]] constexpr bool GreaterEqual(T _lhs, U _rhs) noexcept
{
    if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>)
    {
        return _lhs < 0 ? false : static_cast<std::make_unsigned_t<T>>(_lhs) >= _rhs;
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U>)
    {
        return _rhs < 0 ? true : _lhs >= static_cast<std::make_unsigned_t<U>>(_rhs);
    }
    else
    {
        return _lhs >= _rhs;
    }
}

template<typename T, typename U>
    requires std::is_integral_v<T> && std::is_integral_v<U>
[[nodiscard]] constexpr bool Equal(T _lhs, U _rhs) noexcept
{
    if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>)
    {
        return _lhs < 0 ? false : static_cast<std::make_unsigned_t<T>>(_lhs) == _rhs;
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U>)
    {
        return _rhs < 0 ? false : _lhs == static_cast<std::make_unsigned_t<U>>(_rhs);
    }
    else
    {
        return _lhs == _rhs;
    }
}

template<typename T, typename U>
    requires std::is_integral_v<T> && std::is_integral_v<U>
[[nodiscard]] constexpr bool NotEqual(T _lhs, U _rhs) noexcept
{
    if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>)
    {
        return _lhs < 0 ? true : static_cast<std::make_unsigned_t<T>>(_lhs) != _rhs;
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U>)
    {
        return _rhs < 0 ? true : _lhs != static_cast<std::make_unsigned_t<U>>(_rhs);
    }
    else
    {
        return _lhs != _rhs;
    }
}

}   // namespace Jam