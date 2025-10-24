#pragma once

namespace jam
{

template<typename To, typename From>
    requires std::is_convertible_v<From, To>
constexpr FORCEINLINE To NarrowCast(From value) noexcept
{
    To to = static_cast<To>(value);
    JAM_ASSERT(static_cast<From>(to) == value, "NarrowCast failed: expected {}, got {}", value, to);
    return to;
}

template<typename T>
    requires std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>
JAM_NODISCARD bool MemEqual(const T& _lhs, const T& _rhs) noexcept
{
    return std::memcmp(&_lhs, &_rhs, sizeof(T)) == 0;
}

}   // namespace jam