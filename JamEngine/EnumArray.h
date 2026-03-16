#pragma once
#include "EnumUtilities.h"

namespace Jam
{

// direct indexing enum: index by the underlying enum m_value, just cast to GetSize type.

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

template<typename TEnum>
concept IsDirectIndexableEnum = (std::is_enum_v<TEnum>
                                 && (UnderlyingFrom(MinOf<TEnum>()) == 0)
                                 && (CountOf<TEnum>() > 0)
                                 && (UnderlyingFrom(MaxOf<TEnum>()) == CountOf<TEnum>() - 1));

template<typename TEnum, typename TValue>
class EnumArray
{
public:
    static constexpr SizeT k_cap = CountOf<TEnum>();
    using IterT                  = TValue*;
    using ConstIterT             = const TValue*;

    [[nodiscard]] constexpr TValue& operator[](TEnum _key)
    {
        return m_arr[GetIndex_(_key)];
    }

    [[nodiscard]] constexpr const TValue& operator[](TEnum _key) const
    {
        return m_arr[GetIndex_(_key)];
    }

    [[nodiscard]] constexpr TValue& operator[](SizeT _idx)
    {
        JAM_ASSERT(0 <= _idx && _idx < k_cap, "EnumArray index out of bounds. idx: '{}'", _idx);
        return m_arr[_idx];
    }

    [[nodiscard]] constexpr const TValue& operator[](SizeT _idx) const
    {
        JAM_ASSERT(0 <= _idx && _idx < k_cap, "EnumArray index out of bounds. idx: '{}'", _idx);
        return m_arr[_idx];
    }

    [[nodiscard]] constexpr SizeT Size() const
    {
        return k_cap;
    }

    constexpr void Fill(const TValue& _value)
    {
        for (SizeT i = 0; i < k_cap; ++i)
        {
            m_arr[i] = _value;
        }
    }

    ////////// stl glue //////////

    using value_type      = TValue;
    using size_type       = SizeT;
    using reference       = TValue&;
    using const_reference = const TValue&;
    using iterator        = IterT;
    using const_iterator  = ConstIterT;
    using difference_type = std::ptrdiff_t;

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
        return m_arr + k_cap;
    }

    [[nodiscard]] constexpr ConstIterT end() const
    {
        return m_arr + k_cap;
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
        return k_cap;
    }

    TValue m_arr[k_cap];   // for aggregate initialization and better cache locality. if use std::array, it will add an extra layer of indirection and may cause worse cache performance.

private:
    [[nodiscard]] constexpr SizeT GetIndex_(TEnum _key) const
    {
        if constexpr (IsDirectIndexableEnum<TEnum>)
        {
            const SizeT idx = UnderlyingFrom(_key);
            JAM_ASSERT(0 <= idx && idx < k_cap, "EnumArray index out of bounds. key: '{}'", NameOf(_key));
            return idx;
        }
        else
        {
            const SizeT seq = GetSequence(_key);
            JAM_ASSERT(seq < k_cap, "EnumArray index out of bounds. key: '{}'", NameOf(_key));
            return seq;
        }
    }
};

}   // namespace Jam
