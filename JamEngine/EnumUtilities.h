#pragma once
#define MAGIC_ENUM_RANGE_MIN 0
#include "Result.h"

#include <magic_enum/magic_enum.hpp>

// Enum 유틸리티 함수 정의
// 추적되는 열거형의 범위는 0 ~ 127로 제한됩니다.
// 추적을 피하고 싶은 값은 음수로 설정하는 것을 권장합니다.
// 예: enum class Color { Unknown = -1, Red = 0, Green = 1, Blue = 2 };

namespace jam
{

template<typename E>
JAM_NODISCARD constexpr auto EnumToInt(E _value) noexcept
{
    return magic_enum::enum_integer(_value);
}

template<typename E>
JAM_NODISCARD constexpr std::string_view EnumToString(E _value) noexcept
{
    return magic_enum::enum_name(_value);
}

template<typename E>
JAM_NODISCARD constexpr Result<E> EnumFromInt(std::underlying_type_t<E> _value) noexcept
{
    if (auto e = magic_enum::enum_cast<E>(_value))
    {
        return e.value();
    }
    else
    {
        return eErrorCode::InvalidArgument;
    }
}

template<typename E>
JAM_NODISCARD constexpr E EnumFromInt(std::underlying_type_t<E> _value, E _defaultIfFail) noexcept
{
    auto [ec, v] = EnumFromInt<E>(_value);
    if (IsError(ec))
    {
        return _defaultIfFail;
    }
    return v;
}

template<typename E>
JAM_NODISCARD constexpr Result<E> EnumFromString(const std::string_view _value) noexcept
{
    if (auto e = magic_enum::enum_cast<E>(_value))
    {
        return e.value();
    }
    else
    {
        return eErrorCode::InvalidArgument;
    }
}

template<typename E>
JAM_NODISCARD constexpr E EnumFromString(const std::string_view _value, E _defaultIfFail) noexcept
{
    auto [ec, v] = EnumFromString<E>(_value);
    if (IsError(ec))
    {
        return _defaultIfFail;
    }
    return v;
}

template<typename E>
JAM_NODISCARD constexpr bool IsValidEnum(E _value) noexcept
{
    return magic_enum::enum_contains<E>(_value);
}

template<typename E>
JAM_NODISCARD constexpr auto EnumRange()
{
    return magic_enum::enum_values<E>();
}

template<typename E>
JAM_NODISCARD constexpr auto EnumCount()
{
    return magic_enum::enum_count<E>();
}

}   // namespace jam
