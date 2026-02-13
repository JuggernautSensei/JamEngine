#pragma once

namespace Jam
{

template<typename T>
struct AlwaysFalse : std::false_type
{
};

template<typename T>
constexpr bool AlwaysFalseV = AlwaysFalse<T>::value;

template<typename TEnum>
    requires std::is_enum_v<TEnum>
using UnderlyingT = __underlying_type(TEnum);

};   // namespace Jam