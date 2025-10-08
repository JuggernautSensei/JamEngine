#pragma once

namespace jam
{

template<typename To, typename From>
requires std::is_convertible_v<From, To>
constexpr FORCEINLINE To NarrowCast(From value) noexcept
{
    To to = static_cast<To>(value);
    JAM_ASSERT(static_cast<From>(to) == value, "NarrowCast from: {} to: {} failed", NameOf<From>(), NameOf<To>());
    return to;
}

}