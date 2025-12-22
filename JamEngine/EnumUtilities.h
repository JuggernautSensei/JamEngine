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
    requires std::is_enum_v<E>
[[nodiscard]] constexpr auto IntFrom(E _value) noexcept
{
    return static_cast<EnumIntType<E>>(_value);
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr StringView StringFrom(E _value) noexcept
{
    return magic_enum::enum_name(_value);
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr decltype(auto) EnumFrom(std::underlying_type_t<E> _value) noexcept
{
    return magic_enum::enum_cast<E>(_value);
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr decltype(auto) EnumFrom(const StringView _value) noexcept
{
    return magic_enum::enum_cast<E>(_value);
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr E EnumFromOrAssert(std::underlying_type_t<E> _value) noexcept
{
    const E e = static_cast<E>(_value);
    JAM_ASSERT(IsEnumValid(e), "Invalid enum value: {}", _value);
    return e;
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr bool IsEnumValid(E _value) noexcept
{
    return magic_enum::enum_contains<E>(_value);
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr auto EnumRanges()
{
    return magic_enum::enum_values<E>();
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr auto NumEnums()
{
    return magic_enum::enum_count<E>();
}

}   // namespace jam
