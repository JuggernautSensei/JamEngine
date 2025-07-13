#pragma once

namespace jam
{

namespace detail
{

    class TypeString
    {
    };

}   // namespace detail

template<typename T>
NODISCARD constexpr std::string_view NameOf()
{
    return entt::type_id<T>().name();
}

template<typename T>
NODISCARD constexpr UInt32 HashOf()
{
    return entt::type_id<T>().hash();
}

template<typename T>
constexpr bool IsPlainType = std::is_same_v<T, std::remove_cvref_t<T>>;

}   // namespace jam