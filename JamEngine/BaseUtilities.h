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

template<typename T, USizeT N>
[[nodiscard]] constexpr SizeT CountOf(const T (&)[N]) noexcept
{
    return N;
}

}   // namespace Jam