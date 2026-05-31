#pragma once

namespace Jam
{

template<typename T>
struct AlwaysFalse : std::false_type
{
};

template<typename T>
constexpr bool AlwaysFalseV = AlwaysFalse<T>::value;

template<typename T, typename... TArgs>
constexpr bool AnyOfV = (std::is_same_v<T, TArgs> || ...);

template<typename TEnum>
    requires std::is_enum_v<TEnum>
using UnderlyingT = std::underlying_type_t<TEnum>;

};   // namespace Jam