#pragma once

namespace jam
{

#define JAM_ENUM_STATIC_ASSERT(E) static_assert(requires { E::Min; E::Max; E::Min <= E::Max; }, "Enum type must define Min and Max values.")
#define JAM_ENABLE_ENUM_ITERATOR(E)                    \
    auto begin(E)                                      \
    {                                                  \
        return E::Min;                                 \
    }                                                  \
    auto end(E)                                        \
    {                                                  \
        return static_cast<E>(IntFrom<E>(E::Max) + 1); \
    }                                                  \
    E operator++(E& e)                                 \
    {                                                  \
        e = EnumStep(e, 1);                            \
        return e;                                      \
    }                                                  \
    E operator*(E e)                                   \
    {                                                  \
        return e;                                      \
    }

template<typename E>
using UnderlyingType = std::underlying_type_t<E>;

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr auto IntFrom(E _value) noexcept
{
    JAM_ENUM_STATIC_ASSERT(E);
    return static_cast<UnderlyingType<E>>(_value);
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr Int32 NumEnums() noexcept
{
    JAM_ENUM_STATIC_ASSERT(E);
    return IntFrom(E::Max) - IntFrom(E::Min) + 1;
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr bool IsValidEnum(E _value) noexcept
{
    JAM_ENUM_STATIC_ASSERT(E);
    const UnderlyingType<E> iValue = IntFrom(_value);
    return iValue >= IntFrom(E::Min) && iValue <= IntFrom(E::Max);
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr E EnumFrom(UnderlyingType<E> _value) noexcept
{
    JAM_ENUM_STATIC_ASSERT(E);
    JAM_ASSERT(_value >= IntFrom(E::Min) && _value <= IntFrom(E::Max), "EnumFrom: out of range");
    return static_cast<E>(_value);
}

template<typename E>
    requires std::is_enum_v<E>
[[nodiscard]] constexpr E EnumStep(E e, Int32 _step)
{
    JAM_ENUM_STATIC_ASSERT(E);
    UnderlyingType<E> i = IntFrom(e) + _step;
    JAM_ASSERT(i >= IntFrom(E::Min) && i <= IntFrom(E::Max), "EnumStep: out of range");
    return static_cast<E>(i);
}

}   // namespace jam
