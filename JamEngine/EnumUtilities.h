#pragma once
#include <magic_enum/magic_enum.hpp>

namespace jam
{

template<typename E>
NODISCARD constexpr bool IsValidEnum(const E _enum)
{
    static_assert(std::is_enum_v<E>, "E must be an enum type");
    return magic_enum::enum_contains<E>(_enum);
}

template<typename E>
NODISCARD constexpr E GetFirstValueEnum()
{
    static_assert(std::is_enum_v<E>, "E must be an enum type");
    return magic_enum::enum_values<E>().front();
}

template<typename E>
NODISCARD constexpr E GetValidEnum(const E _enum, const E _valueIfInvalid)
{
    static_assert(std::is_enum_v<E>, "E must be an enum type");
    return magic_enum::enum_contains<E>(_enum) ? _enum : _valueIfInvalid;
}

template<typename E>
NODISCARD constexpr auto EnumToInt(const E _enum)
{
    static_assert(std::is_enum_v<E>, "E must be an enum type");
    return magic_enum::enum_integer<E>(_enum);
}

template<typename E>
NODISCARD constexpr auto EnumFromInt(std::underlying_type_t<E> _value)
{
    return magic_enum::enum_cast<E>(_value);
}

template<typename E>
NODISCARD constexpr std::string_view EnumToString(const E _enum)
{
    static_assert(std::is_enum_v<E>, "E must be an enum type");
    return magic_enum::enum_name(_enum);
}

template<typename E>
NODISCARD constexpr auto StringToEnum(const std::string_view _name)
{
    return magic_enum::enum_cast<E>(_name);
}

}   // namespace jam
