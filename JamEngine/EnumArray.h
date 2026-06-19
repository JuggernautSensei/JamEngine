#pragma once

namespace Jam
{

// enum indexable c-style array

// can not use like
// enum class eColorNonContiguous
// {
//     Red = 1,
//     Green = 2,
//     Blue = 5,
//     Yellow = 10,
// };
// because the underlying values are not contiguous and do not start from 0. This would cause out of bounds access and incorrect indexing.
// but can use like
// enum class eColor
// {
//     Red = 0,
//     Green = 1,
//     Blue = 2,
//     Yellow = 3,
// };
// because the underlying values are contiguous and start from 0. This allows correct indexing and bounds checking.

// direct indexing enum: index by the underlying enum m_value, just cast to GetSize type.
template<typename TEnum>
concept IsDirectIndexableEnum = (std::is_enum_v<TEnum>
                                 && (AsUnderlying(MinOf<TEnum>()) == 0)
                                 && (CountOf<TEnum>() > 0)
                                 && (AsUnderlying(MaxOf<TEnum>()) == CountOf<TEnum>() - 1));

template<typename TEnum, typename TValue>
class EnumArray
{
public:
    static constexpr SizeT kCap = CountOf<TEnum>();
    using IterT                 = TValue*;
    using ConstIterT            = const TValue*;

    [[nodiscard]] constexpr TValue& operator[](TEnum _key)
    {
        return m_arr[GetIndex_(_key)];
    }

    [[nodiscard]] constexpr const TValue& operator[](TEnum _key) const
    {
        return m_arr[GetIndex_(_key)];
    }

    [[nodiscard]] constexpr TValue& operator[](SizeT _index)
    {
        JAM_ASSERT(0 <= _index && _index < kCap, "EnumArray index out of bounds. index: '{}'", _index);
        return m_arr[_index];
    }

    [[nodiscard]] constexpr const TValue& operator[](SizeT _index) const
    {
        JAM_ASSERT(0 <= _index && _index < kCap, "EnumArray index out of bounds. index: '{}'", _index);
        return m_arr[_index];
    }

    [[nodiscard]] constexpr SizeT Size() const
    {
        return kCap;
    }

    constexpr void Fill(const TValue& _value)
    {
        for (SizeT i = 0; i < kCap; ++i)
        {
            m_arr[i] = _value;
        }
    }

public:   // for STL
    using value_type      = TValue;
    using size_type       = SizeT;
    using reference       = TValue&;
    using const_reference = const TValue&;
    using iterator        = IterT;
    using const_iterator  = ConstIterT;
    using difference_type = ptrdiff_t;

    [[nodiscard]] constexpr IterT begin()
    {
        return m_arr;
    }

    [[nodiscard]] constexpr ConstIterT begin() const
    {
        return m_arr;
    }

    [[nodiscard]] constexpr IterT end()
    {
        return m_arr + kCap;
    }

    [[nodiscard]] constexpr ConstIterT end() const
    {
        return m_arr + kCap;
    }

    [[nodiscard]] constexpr TValue* data()
    {
        return m_arr;
    }

    [[nodiscard]] constexpr const TValue* data() const
    {
        return m_arr;
    }

    [[nodiscard]] constexpr SizeT size() const
    {
        return kCap;
    }

private:
    [[nodiscard]] constexpr SizeT GetIndex_(TEnum _key) const
    {
        if constexpr (IsDirectIndexableEnum<TEnum>)
        {
            const SizeT index = AsUnderlying(_key);
            JAM_ASSERT(0 <= index && index < kCap, "EnumArray index out of bounds. key: '{}'", NameOf(_key));
            return index;
        }

        const SizeT seq = AsSequence(_key);
        JAM_ASSERT(seq < kCap, "EnumArray index out of bounds. key: '{}'", NameOf(_key));
        return seq;
    }

public:
    TValue m_arr[kCap];
};

}   // namespace Jam
