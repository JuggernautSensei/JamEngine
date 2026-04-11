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
        {
            m_value |= UnderlyingFrom(flag);
        }
    }

    /* implicit */ constexpr Flags(const UnderlyingT _value) noexcept
        : m_value(_value)
    {
    }

    /* implicit */ constexpr Flags(E _value) noexcept
        : m_value(UnderlyingFrom(_value))
    {
    }

    constexpr void And(Flags _other) noexcept
    {
        m_value &= _other.m_value;
    }

    [[nodiscard]] constexpr Flags Anded(const Flags _other) const noexcept
    {
        Flags result = *this;
        result.And(_other);
        return result;
    }

    constexpr void Or(Flags _other) noexcept
    {
        m_value |= _other.m_value;
    }

    [[nodiscard]] constexpr Flags Ored(const Flags _other) const noexcept
    {
        Flags result = *this;
        result.Or(_other);
        return result;
    }

    constexpr void Xor(Flags _other) noexcept
    {
        m_value ^= _other.m_value;
    }

    [[nodiscard]] constexpr Flags Xored(const Flags _other) const noexcept
    {
        Flags result = *this;
        result.Xor(_other);
        return result;
    }

    [[nodiscard]] constexpr bool IsEnabled(Flags _flags) const noexcept
    {
        return (m_value & _flags.m_value) == _flags.m_value;
    }

    [[nodiscard]] constexpr bool IsAnyEnabled(Flags _flags) const noexcept
    {
        return (m_value & _flags.m_value) != 0;
    }

    [[nodiscard]] constexpr bool IsDisabled(Flags _flags) const noexcept
    {
        return (m_value & _flags.m_value) == 0;
    }

    [[nodiscard]] constexpr bool IsAnyDisabled(Flags _flags) const noexcept
    {
        return (m_value & _flags.m_value) != _flags.m_value;
    }

    constexpr void Enable(Flags _flags) noexcept
    {
        m_value |= _flags.m_value;
    }

    [[nodiscard]] constexpr Flags Enabled(const Flags _flags) const noexcept
    {
        Flags result = *this;
        result.Enable(_flags);
        return result;
    }

    void Disable(Flags _flags) noexcept
    {
        m_value &= ~_flags.m_value;
    }

    [[nodiscard]] constexpr Flags Disabled(const Flags _flags) const noexcept
    {
        Flags result = *this;
        result.Disable(_flags);
        return result;
    }

    void Set(const Flags _flags, const bool _bEnable) noexcept
    {
        if (_bEnable)
        {
            Enable(_flags);
        }
        else
        {
            Disable(_flags);
        }
    }

    [[nodiscard]] constexpr Flags Setted(const Flags _flags, const bool _bEnable) const noexcept
    {
        Flags result = *this;
        result.Set(_flags, _bEnable);
        return result;
    }

    void Toggle(Flags _flags) noexcept
    {
        m_value ^= _flags.m_value;
    }

    [[nodiscard]] constexpr Flags Toggled(const Flags _flags) const noexcept
    {
        Flags result = *this;
        result.Toggle(_flags);
        return result;
    }

    void LeftShift(SizeT _shift) noexcept
    {
        m_value <<= _shift;
    }

    [[nodiscard]] constexpr Flags LeftShifted(const SizeT _shift) const noexcept
    {
        Flags result = *this;
        result.LeftShift(_shift);
        return result;
    }

    void RightShift(SizeT _shift) noexcept
    {
        m_value >>= _shift;
    }

    [[nodiscard]] constexpr Flags RightShifted(const SizeT _shift) const noexcept
    {
        Flags result = *this;
        result.RightShift(_shift);
        return result;
    }

    void Not() noexcept
    {
        m_value = ~m_value;
    }

    [[nodiscard]] constexpr Flags Notted() const noexcept
    {
        Flags result = *this;
        result.Not();
        return result;
    }

    [[nodiscard]] constexpr Flags operator|(const Flags _other) const noexcept
    {
        return Enabled(_other);
    }

    constexpr Flags& operator|=(const Flags _other) noexcept
    {
        Enable(_other);
        return *this;
    }

    [[nodiscard]] constexpr Flags operator&(const Flags _other) const noexcept
    {
        return Anded(_other);
    }

    constexpr Flags& operator&=(const Flags _other) noexcept
    {
        And(_other);
        return *this;
    }

    [[nodiscard]] constexpr Flags operator~() const noexcept
    {
        Flags result;
        result.m_value = ~m_value;
        return result;
    }

    [[nodiscard]] constexpr Flags operator^(const Flags _other) const noexcept
    {
        return Xored(_other);
    }

    constexpr Flags& operator^=(const Flags _other) noexcept
    {
        Xor(_other);
        return *this;
    }

    constexpr bool operator==(const Flags _other) const noexcept
    {
        return m_value == _other.m_value;
    }

    constexpr bool operator!=(const Flags _other) const noexcept
    {
        return m_value != _other.m_value;
    }

    constexpr bool operator!() const noexcept
    {
        return m_value == 0;
    }

    /* implicit */ constexpr operator UnderlyingT() const noexcept
    {
        return m_value;
    }

    [[nodiscard]] constexpr bool IsEmpty() const noexcept
    {
        return m_value == 0;
    }

    constexpr void Clear() noexcept
    {
        m_value = 0;
    }

    [[nodiscard]] constexpr UnderlyingT GetValue() const noexcept
    {
        return m_value;
    }

private:
    UnderlyingT m_value = UnderlyingT { 0 };
};

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator|(E _lhs, E _rhs) noexcept
{
    return Flags<E> { _lhs } | _rhs;
}

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator&(E _lhs, E _rhs) noexcept
{
    return Flags<E> { _lhs } & _rhs;
}

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator^(E _lhs, E _rhs) noexcept
{
    return Flags<E> { _lhs } ^ _rhs;
}

template<IsBitMaskableEnum E>
[[nodiscard]] constexpr Flags<E> operator~(E _value) noexcept
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
    {
        return err;
    }

    _flags = Flags<E>(value);
    return Error::None;
}

}   // namespace Jam
