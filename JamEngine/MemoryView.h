// ReSharper disable CppParameterMayBeConst
#pragma once

namespace Jam
{

template<typename TContainer>
concept IsMemoryViewCompatibleContainer = requires(TContainer c) { c.data(); c.size(); } && std::ranges::contiguous_range<TContainer>;

template<typename T, bool kBIsConst>
concept IsMemoryViewCompatibleValue = (kBIsConst || !std::is_const_v<T>);

// 메모리를 소유하지않는 뷰
template<bool kBIsConst>
class BasicMemoryView
{
public:
    using ByteT = std::conditional_t<kBIsConst, const Byte, Byte>;
    using VoidT = std::conditional_t<kBIsConst, const void, void>;

    BasicMemoryView() = default;

    BasicMemoryView(ByteT* _pData, SizeT _size)
        : m_pMem(_pData)
        , m_byteWidth(_size)
    {
    }

    BasicMemoryView(VoidT* _pData, SizeT _size)
        : m_pMem(static_cast<ByteT*>(_pData))
        , m_byteWidth(_size)
    {
    }

    // Memory → CMemory 암시적 변환 허용, 역방향 차단
    template<bool kBOtherConst>
        requires(kBIsConst >= kBOtherConst)
    /* implicit */ BasicMemoryView(const BasicMemoryView<kBOtherConst>& _other)
        : m_pMem(_other.GetPtr())
        , m_byteWidth(_other.GetByteWidth())
    {}

    // C-style 배열
    template<typename T, SizeT N>
        requires(IsMemoryViewCompatibleValue<T, kBIsConst>)
    /* implicit */ BasicMemoryView(T (&_arr)[N])
        : m_pMem(reinterpret_cast<ByteT*>(_arr))
        , m_byteWidth(sizeof(T) * N)
    {}

    // data()와 size()가 있는 연속된 컨테이너만 허용
    template<typename TContainer>
        requires(IsMemoryViewCompatibleContainer<TContainer> && IsMemoryViewCompatibleValue<typename TContainer::value_type, kBIsConst>)
    /* implicit */ BasicMemoryView(TContainer& _container)
        : m_pMem(reinterpret_cast<ByteT*>(_container.data()))
        , m_byteWidth(sizeof(typename TContainer::value_type) * _container.size())
    {
    }

    [[nodiscard]] ByteT* GetPtr() const
    {
        return m_pMem;
    }

    [[nodiscard]] SizeT GetByteWidth() const
    {
        return m_byteWidth;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_byteWidth == 0;
    }

    [[nodiscard]] BasicMemoryView Slice(const SizeT _offset, const SizeT _length) const
    {
        JAM_ASSERT(_offset <= m_byteWidth, "Offset exceeds memory block size.");
        JAM_ASSERT(_length <= m_byteWidth - _offset, "Length exceeds remaining memory block size after offset.");
        return { m_pMem + _offset, _length };
    }

private:
    ByteT* m_pMem      = nullptr;
    SizeT  m_byteWidth = 0;
};

using MutableMemoryView = BasicMemoryView<false>;
using MemoryView        = BasicMemoryView<true>;

// 일반타입 전용
// 암시적 생성자 사용시 greedy하게 매칭되는 것을 방지하기 위해 외부 함수로 분리

template<typename T>
    requires(IsMemoryViewCompatibleValue<T, true>)
[[nodiscard]] MemoryView AsMemoryView(const T& _value)
{
    return { reinterpret_cast<const Byte*>(&_value), sizeof(T) };
}

template<typename T>
    requires(IsMemoryViewCompatibleValue<T, false>)
[[nodiscard]] MutableMemoryView AsMutableMemoryView(T& _value)
{
    return { reinterpret_cast<Byte*>(&_value), sizeof(T) };
}

}   // namespace Jam