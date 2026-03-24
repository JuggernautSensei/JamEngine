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
using UnderlyingT = __underlying_type(TEnum);

template<typename T, typename U>
concept IsMemCpyable = std::is_trivially_copyable_v<T> && std::is_trivially_copyable_v<U> && sizeof(T) == sizeof(U) && std::is_layout_compatible_v<T, U>;

};   // namespace Jam