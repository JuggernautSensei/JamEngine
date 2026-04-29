// ReSharper disable CppClangTidyBugproneMacroParentheses
#pragma once
#include "Archive.h"

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
            m_value |= UnderlyingFrom(flag);
    }

    /* implicit */ constexpr Flags(const UnderlyingT _value) noexcept
        : m_value(_value)
    {
    }

    /* implicit */ constexpr Flags(const E _value) noexcept
        : m_value(UnderlyingFrom(_value))
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
        _bEnable ? Enable(_flags) : Disable(_flags);
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

    [[nodiscard]] constexpr Flags WithEnabled(const Flags _flags) const noexcept
    {
        Flags result = *this;
        result.Enable(_flags);
        return result;
    }

    [[nodiscard]] constexpr Flags WithDisabled(const Flags _flags) const noexcept
    {
        Flags result = *this;
        result.Disable(_flags);
        return result;
    }

    [[nodiscard]] constexpr Flags WithSet(const Flags _flags, const bool _bEnable) const noexcept
    {
        Flags result = *this;
        result.Set(_flags, _bEnable);
        return result;
    }

    [[nodiscard]] constexpr Flags WithToggled(const Flags _flags) const noexcept
    {
        Flags result = *this;
        result.Toggle(_flags);
        return result;
    }

    [[nodiscard]] constexpr Flags WithNot() const noexcept
    {
        Flags result = *this;
        result.Not();
        return result;
    }

    [[nodiscard]] constexpr Flags WithAnd(const Flags _other) const noexcept
    {
        Flags result = *this;
        result.And(_other);
        return result;
    }

    [[nodiscard]] constexpr Flags WithOr(const Flags _other) const noexcept
    {
        Flags result = *this;
        result.Or(_other);
        return result;
    }

    [[nodiscard]] constexpr Flags WithXor(const Flags _other) const noexcept
    {
        Flags result = *this;
        result.Xor(_other);
        return result;
    }

    [[nodiscard]] constexpr Flags WithLeftShift(const SizeT _shift) const noexcept
    {
        Flags result = *this;
        result.LeftShift(_shift);
        return result;
    }

    [[nodiscard]] constexpr Flags WithRightShift(const SizeT _shift) const noexcept
    {
        Flags result = *this;
        result.RightShift(_shift);
        return result;
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

template<typename E>
void WriteToArchive(ArchiveWriter& _writer, const Flags<E> _flags)
{
    _writer.Write(_flags.GetValue());
}

template<typename E>
Error ReadFromArchive(ArchiveReader& _reader, Flags<E>& _flags)
{
    typename Flags<E>::UnderlyingT value = 0;
    if (const Error err = _reader.Read(value))
        return err;

    _flags = Flags<E>(value);
    return Error::None;
}

}   // namespace Jam