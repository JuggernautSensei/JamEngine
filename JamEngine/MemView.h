#pragma once

namespace jam
{

template<bool Const>
class BasicMemView
{
public:
    using ByteT    = std::conditional_t<Const, const Byte, Byte>;
    using VoidPtrT = std::conditional_t<Const, const void*, void*>;

    BasicMemView()
    {
    }

    // raw pointer
    /* implicit */ BasicMemView(VoidPtrT _pData, const Int32 _byteWidth)
        : m_pData(static_cast<ByteT*>(_pData))
        , m_byteWidth(_byteWidth)
    {
    }

    // non-const to const conversion
    /* implicit */ BasicMemView(const BasicMemView<false>& other)
        : m_pData(other.GetPtr())
        , m_byteWidth(other.GetByteWidth())
    {
    }

    // pod type (single variable)
    template<typename T>
        requires std::is_standard_layout_v<T>
                  && (!std::is_pointer_v<T>)
                  && (std::is_const_v<ByteT> || !std::is_const_v<T>)
    /* implicit */ BasicMemView(T& _value)
        : m_pData(reinterpret_cast<ByteT*>(&_value))
        , m_byteWidth(sizeof(T))
    {
    }

    // contiguous container
    template<std::ranges::contiguous_range Range>
        requires(std::is_const_v<ByteT> || !std::is_const_v<std::ranges::range_reference_t<Range>>)
    /* implicit */ BasicMemView(Range&& r)
        : m_pData(reinterpret_cast<ByteT*>(std::ranges::data(r)))
        , m_byteWidth(std::ranges::size(r) * sizeof(std::ranges::range_value_t<Range>))
    {
    }

    // c-style array
    template<typename T, size_t N>
    /* implicit */ BasicMemView(T (&_array)[N])
        : m_pData(reinterpret_cast<ByteT*>(_array))
        , m_byteWidth(N * sizeof(T))
    {
    }

    [[nodiscard]] ByteT* GetPtr() const
    {
        return m_pData;
    }

    [[nodiscard]] Int32 GetByteWidth() const
    {
        return m_byteWidth;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_pData != nullptr;
    }

private:
    ByteT* m_pData     = nullptr;
    Int32  m_byteWidth = 0;
};

using MemView  = BasicMemView<false>;
using CMemView = BasicMemView<true>;

}   // namespace jam
