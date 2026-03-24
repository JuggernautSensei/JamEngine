// ReSharper disable CppParameterMayBeConst
#pragma once

namespace Jam
{

template<typename TContainer>
concept MemoryCompatibleContainer = requires(TContainer c) { c.data(); c.size(); } && std::ranges::contiguous_range<TContainer>;

template<typename T, bool k_bIsConst>
concept MemoryCompatibleValue = (k_bIsConst || !std::is_const_v<T>);

// Byte 단위의 메모리를 표현하는 경량 클래스
// Byte*, SizeT 형태로 표현되는 메모리 블록을 래핑하여,
// 메모리 블록의 시작 주소와 크기를 함께 관리할 수 있도록 함.
template<bool k_bIsConst>
class BasicMemoryView
{
public:
    using ByteT = std::conditional_t<k_bIsConst, const Byte, Byte>;
    using VoidT = std::conditional_t<k_bIsConst, const void, void>;

    BasicMemoryView() = default;

    BasicMemoryView(ByteT* _pData, SizeT _size)
        : m_pData(_pData)
        , m_size(_size)
    {
    }

    BasicMemoryView(VoidT* _pData, SizeT _size)
        : m_pData(static_cast<ByteT*>(_pData))
        , m_size(_size)
    {
    }

    // Memory → CMemory 암시적 변환 허용, 역방향 차단
    template<bool k_bOtherConst>
        requires(k_bIsConst >= k_bOtherConst)
    /* implicit */ BasicMemoryView(const BasicMemoryView<k_bOtherConst>& _other)
        : m_pData(_other.GetPtr())
        , m_size(_other.GetByteWidth())
    {}

    // C-style 배열
    template<typename T, SizeT N>
        requires(MemoryCompatibleValue<T, k_bIsConst>)
    /* implicit */ BasicMemoryView(T (&_arr)[N])
        : m_pData(reinterpret_cast<ByteT*>(_arr))
        , m_size(sizeof(T) * N)
    {}

    // data()와 size()가 있는 연속된 컨테이너만 허용
    template<typename TContainer>
        requires(MemoryCompatibleContainer<TContainer> && MemoryCompatibleValue<typename TContainer::value_type, k_bIsConst>)
    /* implicit */ BasicMemoryView(TContainer& _container)
        : m_pData(reinterpret_cast<ByteT*>(_container.data()))
        , m_size(sizeof(typename TContainer::value_type) * _container.size())
    {
    }

    [[nodiscard]] ByteT* GetPtr() const
    {
        return m_pData;
    }

    [[nodiscard]] SizeT GetByteWidth() const
    {
        return m_size;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_size == 0;
    }

    [[nodiscard]] BasicMemoryView Slice(const SizeT _offset, const SizeT _length) const
    {
        JAM_ASSERT(_offset <= m_size, "Offset exceeds memory block size.");
        JAM_ASSERT(_length <= m_size - _offset, "Length exceeds remaining memory block size after offset.");
        return { m_pData + _offset, _length };
    }

private:
    ByteT* m_pData = nullptr;
    SizeT  m_size  = 0;
};

using MutableMemoryView = BasicMemoryView<false>;
using MemoryView        = BasicMemoryView<true>;

// 일반타입 전용
// 암시적 생성자 사용시 greedy하게 매칭되는 것을 방지하기 위해 외부 함수로 분리

template<typename T>
    requires(MemoryCompatibleValue<T, true>)
[[nodiscard]] MemoryView AsMemoryView(const T& _value)
{
    return { reinterpret_cast<const Byte*>(&_value), sizeof(T) };
}

template<typename T>
    requires(MemoryCompatibleValue<T, false>)
[[nodiscard]] MutableMemoryView AsMutableMemoryView(T& _value)
{
    return { reinterpret_cast<Byte*>(&_value), sizeof(T) };
}

}   // namespace Jam