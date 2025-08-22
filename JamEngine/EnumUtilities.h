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
NODISCARD constexpr E GetValidEnumOrDefault(const E _enum, const E _valueIfInvalid)
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
NODISCARD constexpr auto EnumCount()
{
    static_assert(std::is_enum_v<E>, "E must be an enum type");
    return magic_enum::enum_count<E>();
}

template<typename E>
NODISCARD constexpr Result<E> EnumFromInt(std::underlying_type_t<E> _value)
{
    if (magic_enum::enum_contains<E>(_value))
    {
        return static_cast<E>(_value);   // If the value is valid, return it as E
    }
    else
    {
        return Fail;   // If the value is invalid, return Fail
    }
}

template<typename E>
NODISCARD constexpr std::string_view EnumToString(const E _enum)
{
    static_assert(std::is_enum_v<E>, "E must be an enum type");
    return magic_enum::enum_name(_enum);
}

template<typename E>
NODISCARD constexpr auto EnumRange()
{
    return magic_enum::enum_values<E>();
}

}   // namespace jam
