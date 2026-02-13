#pragma once

namespace Jam
{

template<typename E>
    requires std::is_enum_v<E>
          && std::is_unsigned_v<UnderlyingT<E>>
class Flags
{
    using UnderlyingType = UnderlyingT<E>;
    using UnsignedType   = std::make_unsigned_t<UnderlyingType>;

public:
    constexpr Flags() = default;

    /* implicit */ constexpr Flags(E _value) noexcept
        : m_value(UnderlyingFrom(_value))
    {
    }

    /* implicit */ constexpr Flags(const std::initializer_list<E> _list) noexcept
    {
        for (E val: _list)
        {
            m_value |= UnderlyingFrom(val);
        }
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

    [[nodiscard]] constexpr bool IsDisabled(Flags _flags) const noexcept
    {
        return (m_value & _flags.m_value) == 0;
    }

    void Enable(Flags _flags) noexcept
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

    void LeftShift(Int32 _shift) noexcept
    {
        m_value <<= _shift;
    }

    [[nodiscard]] constexpr Flags LeftShifted(const Int32 _shift) const noexcept
    {
        Flags result = *this;
        result.LeftShift(_shift);
        return result;
    }

    void RightShift(Int32 _shift) noexcept
    {
        m_value >>= _shift;
    }

    [[nodiscard]] constexpr Flags RightShifted(const Int32 _shift) const noexcept
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

    [[nodiscard]] constexpr Flags operator<<(const Int32 _other) const noexcept
    {
        return LeftShifted(_other);
    }

    constexpr Flags& operator<<=(const Int32 _other) noexcept
    {
        LeftShift(_other);
        return *this;
    }

    [[nodiscard]] constexpr Flags operator>>(const Int32 _other) const noexcept
    {
        return RightShifted(_other);
    }

    constexpr Flags& operator>>=(const Int32 _other) noexcept
    {
        RightShift(_other);
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

    /* implicit */ constexpr operator bool() const noexcept
    {
        return m_value != 0;
    }

    /* implicit */ constexpr operator UnsignedType() const noexcept
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

    [[nodiscard]] constexpr UnsignedType GetValue() const noexcept
    {
        return m_value;
    }

private:
    UnsignedType m_value = UnsignedType { 0 };
};

template<typename E>
    requires std::is_enum_v<E>
Flags<E> MakeFlags(std::initializer_list<E> _list) noexcept
{
    return Flags<E> { _list };
}

}   // namespace Jam
