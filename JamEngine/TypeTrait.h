#pragma once

namespace jam
{

template<typename T>
[[nodiscard]] constexpr UInt32 HashOf()
{
    return entt::type_hash<T>::value();
}

template<typename T>
constexpr bool AlwaysFalse = false;

}   // namespace jam