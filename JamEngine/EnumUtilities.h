#pragma once
#define MAGIC_ENUM_RANGE_MIN 0

#include <magic_enum/magic_enum.hpp>

// Enum 유틸리티 함수 정의
// 추적되는 열거형의 범위는 0 ~ 127로 제한됩니다.
// 추적을 피하고 싶은 값은 음수로 설정하는 것을 권장합니다.
// 예: enum class Color { Unknown = -1, Red = 0, Green = 1, Blue = 2 };

namespace jam
{

template<typename E>
using EnumIntType = std::underlying_type_t<E>;

template<typename E>
[[nodiscard]] constexpr auto EnumToInt(E _value) noexcept
{
    return magic_enum::enum_integer(_value);
}

template<typename E>
[[nodiscard]] constexpr std::string_view EnumToString(E _value) noexcept
{
    return magic_enum::enum_name(_value);
}

template<typename E>
[[nodiscard]] constexpr decltype(auto) EnumFromInt(std::underlying_type_t<E> _value) noexcept
{
    return magic_enum::enum_cast<E>(_value);
}

template<typename E>
[[nodiscard]] constexpr decltype(auto) EnumFromString(const std::string_view _value) noexcept
{
    return magic_enum::enum_cast<E>(_value);
}

template<typename E>
[[nodiscard]] constexpr bool IsValidEnum(E _value) noexcept
{
    return magic_enum::enum_contains<E>(_value);
}

template<typename E>
[[nodiscard]] constexpr auto EnumRange()
{
    return magic_enum::enum_values<E>();
}

template<typename E>
[[nodiscard]] constexpr auto EnumCount()
{
    return magic_enum::enum_count<E>();
}

}   // namespace jam
