// ReSharper disable CppClangTidyBugproneMacroParentheses
#pragma once
#include "TypeTraits.h"

namespace Jam
{

template<typename E>
concept IsBitMaskableEnum = std::is_enum_v<E> && std::is_unsigned_v<UnderlyingT<E>>;

template<IsBitMaskableEnum E>
class Flags
{
public:
    using UnderlyingT = UnderlyingT<E>;

    constexpr Flags() = default;

    /* implicit */ constexpr Flags(std::initializer_list<E> _flags) noexcept
        : m_value(0)
    {
        for (const E flag: _flags)
        {
            m_value |= AsUnderlying(flag);
        }
    }

    /* implicit */ constexpr Flags(const UnderlyingT _value) noexcept
        : m_value(_value)
    {
    }

    /* implicit */ constexpr Flags(const E _value) noexcept
        : m_value(AsUnderlying(_value))
    {
    }

    [[nodiscard]] constexpr bool IsEmpty() const noexcept
    {
        return m_value == 0;
    }

    [[nodiscard]] constexpr bool IsEnabled(const Flags _flags) const noexcept
    {
        return (m_value & _flags.m_value) == _flags.m_value;
    }

    [[nodiscard]] constexpr bool IsAnyEnabled(const Flags _flags) const noexcept
    {
        return (m_value & _flags.m_value) != 0;
    }

    [[nodiscard]] constexpr bool IsDisabled(const Flags _flags) const noexcept
    {
        return (m_value & _flags.m_value) == 0;
    }

    [[nodiscard]] constexpr bool IsAnyDisabled(const Flags _flags) const noexcept
    {
        return (m_value & _flags.m_value) != _flags.m_value;
    }

    [[nodiscard]] constexpr UnderlyingT GetValue() const noexcept
    {
        return m_value;
    }

    constexpr void Clear() noexcept
    {
        m_value = 0;
    }

    constexpr void Enable(const Flags _flags) noexcept
    {
        m_value |= _flags.m_value;
    }

    constexpr void Disable(const Flags _flags) noexcept
    {
        m_value &= ~_flags.m_value;
    }

    constexpr void Set(const Flags _flags, const bool _bEnable) noexcept
    {
        const UnderlyingT mask    = _flags.m_value;
        const UnderlyingT enabler = static_cast<UnderlyingT>(0) - static_cast<UnderlyingT>(_bEnable);
        m_value                   = (m_value & ~mask) | (mask & enabler);
    }

    constexpr void Toggle(const Flags _flags) noexcept
    {
        m_value ^= _flags.m_value;
    }

    constexpr void Not() noexcept
    {
        m_value = ~m_value;
    }

    constexpr void And(const Flags _other) noexcept
    {
        m_value &= _other.m_value;
    }

    constexpr void Or(const Flags _other) noexcept
    {
        m_value |= _other.m_value;
    }

    constexpr void Xor(const Flags _other) noexcept
    {
        m_value ^= _other.m_value;
    }

    constexpr void LeftShift(const SizeT _shift) noexcept
    {
        m_value <<= _shift;
    }

    constexpr void RightShift(const SizeT _shift) noexcept
    {
        m_value >>= _shift;
    }

    constexpr void Filter(const UnderlyingT _mask) noexcept
    {
        m_value &= _mask;
    }

    constexpr void Filter(const E _low, const E _high) noexcept
    {
        m_value &= MakeMask_(_low, _high);
    }

    [[nodiscard]] constexpr Flags WithEnabled(const Flags _flags) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value | _flags.m_value) };
    }

    [[nodiscard]] constexpr Flags WithDisabled(const Flags _flags) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value & ~_flags.m_value) };
    }

    [[nodiscard]] constexpr Flags WithSet(const Flags _flags, const bool _bEnable) const noexcept
    {
        const UnderlyingT mask    = _flags.m_value;
        const UnderlyingT enabler = static_cast<UnderlyingT>(0) - static_cast<UnderlyingT>(_bEnable);
        return Flags { static_cast<UnderlyingT>((m_value & ~mask) | (mask & enabler)) };
    }

    [[nodiscard]] constexpr Flags WithToggled(const Flags _flags) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value ^ _flags.m_value) };
    }

    [[nodiscard]] constexpr Flags WithNot() const noexcept
    {
        return Flags { static_cast<UnderlyingT>(~m_value) };
    }

    [[nodiscard]] constexpr Flags WithAnd(const Flags _other) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value & _other.m_value) };
    }

    [[nodiscard]] constexpr Flags WithOr(const Flags _other) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value | _other.m_value) };
    }

    [[nodiscard]] constexpr Flags WithXor(const Flags _other) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value ^ _other.m_value) };
    }

    [[nodiscard]] constexpr Flags WithLeftShift(const SizeT _shift) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value << _shift) };
    }

    [[nodiscard]] constexpr Flags WithRightShift(const SizeT _shift) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value >> _shift) };
    }

    [[nodiscard]] constexpr Flags WithFilter(const UnderlyingT _mask) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value & _mask) };
    }

    [[nodiscard]] constexpr Flags WithFilter(const E _low, const E _high) const noexcept
    {
        return Flags { static_cast<UnderlyingT>(m_value & MakeMask_(_low, _high)) };
    }

    [[nodiscard]] constexpr Flags operator|(const Flags _other) const noexcept
    {
        return WithEnabled(_other);
    }

    constexpr Flags& operator|=(const Flags _other) noexcept
    {
        Enable(_other);
        return *this;
    }

    [[nodiscard]] constexpr Flags operator&(const Flags _other) const noexcept
    {
        return WithAnd(_other);
    }

    constexpr Flags& operator&=(const Flags _other) noexcept
    {
        And(_other);
        return *this;
    }

    [[nodiscard]] constexpr Flags operator^(const Flags _other) const noexcept
    {
        return WithXor(_other);
    }

    constexpr Flags& operator^=(const Flags _other) noexcept
    {
        Xor(_other);
        return *this;
    }

    [[nodiscard]] constexpr Flags operator~() const noexcept
    {
        return WithNot();
    }

    [[nodiscard]] constexpr bool operator==(const Flags _other) const noexcept
    {
        return m_value == _other.m_value;
    }

    [[nodiscard]] constexpr bool operator!=(const Flags _other) const noexcept
    {
        return m_value != _other.m_value;
    }

    [[nodiscard]] constexpr bool operator!() const noexcept
    {
        return m_value == 0;
    }

    constexpr explicit operator bool() const noexcept
    {
        return m_value != 0;
    }

private:
    [[nodiscard]] static constexpr UnderlyingT MakeMask_(const E _low, const E _high) noexcept
    {
        const UnderlyingT low  = AsUnderlying(_low);
        const UnderlyingT high = AsUnderlying(_high);
        JAM_ASSERT(low <= high, "Low value must be less than or equal to high value");
        return ((UnderlyingT { 1 } << (high - low + 1)) - 1) << low;
    }

    UnderlyingT m_value = UnderlyingT { 0 };
};

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator|(const E _lhs, const E _rhs) noexcept
{
    return Flags<E> { _lhs } | _rhs;
}

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator&(const E _lhs, const E _rhs) noexcept
{
    return Flags<E> { _lhs } & _rhs;
}

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator^(const E _lhs, const E _rhs) noexcept
{
    return Flags<E> { _lhs } ^ _rhs;
}

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator~(const E _value) noexcept
{
    return ~Flags<E> { _value };
}

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator|(const E _lhs, const Flags<E> _rhs) noexcept
{
    return Flags<E> { _lhs } | _rhs;
}

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator&(const E _lhs, const Flags<E> _rhs) noexcept
{
    return Flags<E> { _lhs } & _rhs;
}

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator^(const E _lhs, const Flags<E> _rhs) noexcept
{
    return Flags<E> { _lhs } ^ _rhs;
}

}   // namespace Jam