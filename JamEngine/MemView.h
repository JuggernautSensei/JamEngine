#pragma once

namespace jam
{

template<bool Const>
class BasicMemView
{
public:
    using TByte    = std::conditional_t<Const, const Byte, Byte>;
    using TVoidPtr = std::conditional_t<Const, const void*, void*>;

    BasicMemView()
    {
    }

    // raw pointer
    /* implicit */ BasicMemView(TVoidPtr _pData, const size_t _byteWidth)
        : pData(static_cast<TByte*>(_pData))
        , byteWidth(_byteWidth)
    {
    }

    // non-const to const conversion
    /* implicit */ BasicMemView(const BasicMemView<false>& other)
        : pData(other.GetDataPtr())
        , byteWidth(other.GetByteWidth())
    {
    }

    // pod type (single variable)
    template<typename T>
        requires std::is_standard_layout_v<T>
                  && (!std::is_pointer_v<T>)
                  && (std::is_const_v<TByte> || !std::is_const_v<T>)
    /* implicit */ BasicMemView(T& _value)
        : pData(reinterpret_cast<TByte*>(&_value))
        , byteWidth(sizeof(T))
    {
    }

    // contiguous container
    template<std::ranges::contiguous_range Range>
        requires(std::is_const_v<TByte> || !std::is_const_v<std::ranges::range_reference_t<Range>>)
    /* implicit */ BasicMemView(Range&& r)
        : pData(reinterpret_cast<TByte*>(std::ranges::data(r)))
        , byteWidth(std::ranges::size(r) * sizeof(std::ranges::range_value_t<Range>))
    {
    }

    // c-style array
    template<typename T, size_t N>
    /* implicit */ BasicMemView(T (&_array)[N])
        : pData(reinterpret_cast<TByte*>(_array))
        , byteWidth(N * sizeof(T))
    {
    }

    [[nodiscard]] TByte* GetDataPtr() const
    {
        return pData;
    }

    [[nodiscard]] size_t GetByteWidth() const
    {
        return byteWidth;
    }

    [[nodiscard]] bool IsValid() const
    {
        return pData != nullptr;
    }

private:
    TByte* pData     = nullptr;
    size_t byteWidth = 0;
};

using MemView      = BasicMemView<false>;
using ConstMemView = BasicMemView<true>;

}   // namespace jam
