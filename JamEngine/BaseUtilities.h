#pragma once

namespace jam
{

#ifndef JAM_NEW
#    define JAM_NEW(_type, ...) new _type(__VA_ARGS__)
#endif

#ifndef JAM_DELETE
#    define JAM_DELETE(_ptr)  \
        JAM_BEGIN_MACRO_BLOCK \
        delete (_ptr);        \
        (_ptr) = nullptr;     \
        JAM_END_MACRO_BLOCK
#endif

#ifndef JAM_DELETE_ARRAY
#    define JAM_DELETE_ARRAY(_ptr) \
        JAM_BEGIN_MACRO_BLOCK      \
        delete[] (_ptr);           \
        (_ptr) = nullptr;          \
        JAM_END_MACRO_BLOCK
#endif

template<typename To, typename From>
    requires std::is_convertible_v<From, To>
constexpr FORCEINLINE To NarrowCast(From value) noexcept
{
    const To to = static_cast<To>(value);
    JAM_ASSERT(static_cast<From>(to) == value, "NarrowCast failed: expected {}, got {}", value, to);
    return to;
}

template<typename T>
    requires std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>
[[nodiscard]] bool MemEqual(const T& _lhs, const T& _rhs) noexcept
{
    return std::memcmp(&_lhs, &_rhs, sizeof(T)) == 0;
}

template<typename T, typename R>
    requires std::is_integral_v<T> && std::convertible_to<R, T>
void SetBit(T& _value, R _bit, const bool _bSet) noexcept
{
    if (_bSet)
    {
        _value |= static_cast<T>(_bit);
    }
    else
    {
        _value &= ~static_cast<T>(_bit);
    }
}

template<typename T, typename R>
[[nodiscard]] bool CmpLess(const T& _a, const R& _b) noexcept
{
    using CommonType = std::common_type_t<T, R>;
    return static_cast<CommonType>(_a) < static_cast<CommonType>(_b);
}

template<typename T, typename R>
[[nodiscard]] bool CmpGreater(const T& _a, const R& _b) noexcept
{
    using CommonType = std::common_type_t<T, R>;
    return static_cast<CommonType>(_a) > static_cast<CommonType>(_b);
}

template<typename T, typename R>
[[nodiscard]] bool CmpLessEqual(const T& _a, const R& _b) noexcept
{
    using CommonType = std::common_type_t<T, R>;
    return static_cast<CommonType>(_a) <= static_cast<CommonType>(_b);
}

template<typename T, typename R>
[[nodiscard]] bool CmpGreaterEqual(const T& _a, const R& _b) noexcept
{
    using CommonType = std::common_type_t<T, R>;
    return static_cast<CommonType>(_a) >= static_cast<CommonType>(_b);
}

template<typename T, typename R>
[[nodiscard]] bool CmpEqual(const T& _a, const R& _b) noexcept
{
    using CommonType = std::common_type_t<T, R>;
    return static_cast<CommonType>(_a) == static_cast<CommonType>(_b);
}

template<typename T, typename R>
[[nodiscard]] bool CmpNotEqual(const T& _a, const R& _b) noexcept
{
    using CommonType = std::common_type_t<T, R>;
    return static_cast<CommonType>(_a) != static_cast<CommonType>(_b);
}


}   // namespace jam