// ReSharper disable CppParameterMayBeConst
#pragma once

#include <ranges>
#include <type_traits>

// 고전적인 const void* _ptr, size_t _size 조합을 캡슐화한 클래스
// 다양한 소스에서 메모리 블록을 쉽게 표현하고 전달할 수 있도록 도와줌
// mutable 버전과 const 버전 두 가지 제공
// 가능하다면 템플릿을 우선적으로 사용하되
// c++ 환경에서 void* 과 size_t 조합을 직접 사용해야하는 경우에만 이 클래스를 사용
// 대부분의 용도는 C 스타일 API와의 상호작용, 타입없는 순수 바이트 스트림 처리 등

namespace Jam
{

template<bool k_bIsConst>
class BasicMemoryBlock
{
public:
    using ByteT    = std::conditional_t<k_bIsConst, const Byte, Byte>;
    using BytePtrT = ByteT*;
    using VoidPtrT = std::conditional_t<k_bIsConst, const void*, void*>;

    constexpr BasicMemoryBlock() = default;

    constexpr BasicMemoryBlock(
        BytePtrT    _pData,
        const SizeT _byteWidth)
        : m_pData(_pData)
        , m_size(_byteWidth)
    {
    }

    // void* 기반 생성자
    BasicMemoryBlock(
        VoidPtrT    _pData,
        const SizeT _byteWidth)
        : m_pData(reinterpret_cast<BytePtrT>(_pData))
        , m_size(_byteWidth)
    {
    }

    // Non-MemoryView -> CMemoryView 변환
    /* implicit */ BasicMemoryBlock(
        const BasicMemoryBlock<false>& _other)
        : m_pData(_other.GetPtr())
        , m_size(_other.GetByteWidth())
    {
    }

    // C-style 배열
    template<typename U, std::size_t N>
    /* implicit */ BasicMemoryBlock(U (&_array)[N])
        : m_pData(reinterpret_cast<BytePtrT>(_array))
        , m_size(NumericCast<SizeT>(N * sizeof(U)))
    {
    }

    // C-style 배열
    template<typename U, std::size_t N>
        requires(k_bIsConst)
    /* implicit */ BasicMemoryBlock(const U (&_array)[N])
        : m_pData(reinterpret_cast<BytePtrT>(_array))
        , m_size(NumericCast<SizeT>(N * sizeof(U)))
    {
    }

    // STL 컨테이너
    template<typename TContainer>
        requires(std::ranges::contiguous_range<TContainer>)
    /* implicit */ BasicMemoryBlock(TContainer& _container)
        : m_pData(reinterpret_cast<BytePtrT>(_container.data()))
        , m_size(NumericCast<SizeT>(_container.size() * sizeof(typename TContainer::value_type)))
    {
    }

    // STL 컨테이너
    template<typename TContainer>
        requires(k_bIsConst && std::ranges::contiguous_range<TContainer>)
    /* implicit */ BasicMemoryBlock(const TContainer& _container)
        : m_pData(reinterpret_cast<BytePtrT>(_container.data()))
        , m_size(NumericCast<SizeT>(_container.size() * sizeof(typename TContainer::value_type)))
    {
    }

    // 단일 객체
    template<typename U>
        requires(!std::ranges::contiguous_range<U>)
    /* implicit */ BasicMemoryBlock(U& _data)
        : m_pData(reinterpret_cast<BytePtrT>(&_data))
        , m_size(NumericCast<SizeT>(sizeof(U)))
    {
    }

    // 단일 객체
    template<typename U>
        requires(k_bIsConst && !std::ranges::contiguous_range<U>)
    /* implicit */ BasicMemoryBlock(const U& _data)
        : m_pData(reinterpret_cast<BytePtrT>(&_data))
        , m_size(NumericCast<SizeT>(sizeof(U)))
    {
    }

    [[nodiscard]] BytePtrT GetPtr() const
    {
        return m_pData;
    }

    [[nodiscard]] SizeT GetByteWidth() const
    {
        return m_size;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_pData == nullptr || m_size == 0;
    }

    [[nodiscard]] BasicMemoryBlock SubBlock(
        const SizeT _offset) const
    {
        return SubBlock(_offset, m_size > _offset ? m_size - _offset : 0);
    }

    [[nodiscard]] BasicMemoryBlock SubBlock(
        const SizeT _offset,
        const SizeT _byteWidth) const
    {
        JAM_ASSERT(_offset <= m_size, "SubBlock offset out of bounds.");
        JAM_ASSERT(_offset + _byteWidth <= m_size, "SubBlock byte width out of bounds.");
        return { m_pData + _offset, _byteWidth };
    }

private:
    BytePtrT m_pData = nullptr;
    SizeT    m_size  = 0;
};

using MemoryBlock  = BasicMemoryBlock<false>;
using CMemoryBlock = BasicMemoryBlock<true>;

}   // namespace Jam