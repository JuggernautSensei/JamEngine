#pragma once

// 고전적인 const void* _ptr, size_t _size 조합을 캡슐화한 클래스
// 다양한 소스에서 메모리 블록을 쉽게 표현하고 전달할 수 있도록 도와줌
// mutable 버전과 const 버전 두 가지 제공
// 가능하다면 템플릿을 우선적으로 사용하되
// c++ 환경에서 void* 과 size_t 조합을 직접 사용해야하는 경우에만 이 클래스를 사용
// 대부분의 용도는 C 스타일 API와의 상호작용, 타입없는 순수 바이트 스트림 처리 등

namespace Jam
{

class MemoryBlock
{
public:
    MemoryBlock() = default;
    MemoryBlock(Byte* _pData, SizeT _byteWidth);
    MemoryBlock(void* _pData, SizeT _byteWidth);

    // 연속 컨테이너
    template<typename TContainer>
        requires(std::ranges::contiguous_range<TContainer>
                 && !std::is_const_v<std::remove_reference_t<decltype(*std::declval<TContainer>().data())>>
                 && !std::is_const_v<TContainer>)
    /* implicit */ MemoryBlock(TContainer& _container)
        : m_pData(reinterpret_cast<Byte*>(_container.data()))
        , m_size(NumericCast<SizeT>(_container.size() * sizeof(TContainer::value_type)))
    {
    }

    // C-style 배열
    template<typename T, std::size_t N>
        requires(!std::is_const_v<T>)
    /* implicit */ MemoryBlock(T (&_array)[N])
        : m_pData(reinterpret_cast<Byte*>(_array))
        , m_size(NumericCast<SizeT>(N * sizeof(T)))
    {
    }

    template<typename T>
        requires(!std::ranges::range<T>
                 && !std::is_array_v<T>
                 && std::is_standard_layout_v<T>
                 && !std::is_pointer_v<T>
                 && !std::is_const_v<T>)
    /* implicit */ MemoryBlock(T& _data)
        : m_pData(reinterpret_cast<Byte*>(&_data))
        , m_size(NumericCast<SizeT>(sizeof(T)))
    {
    }

    [[nodiscard]] Byte*       GetPtr() const;
    [[nodiscard]] SizeT       GetByteWidth() const;
    [[nodiscard]] MemoryBlock SubBlock(SizeT _offset) const;
    [[nodiscard]] MemoryBlock SubBlock(SizeT _offset, SizeT _byteWidth) const;
    [[nodiscard]] bool        IsEmpty() const;

private:
    Byte* m_pData = nullptr;
    SizeT m_size  = 0;
};

class CMemoryBlock
{
public:
    CMemoryBlock() = default;
    CMemoryBlock(const Byte* _pData, SizeT _byteWidth);
    CMemoryBlock(const void* _pData, SizeT _byteWidth);
    /* implicit */ CMemoryBlock(MemoryBlock _mem);

    // 연속 컨테이너
    template<typename TContainer>
        requires(std::ranges::contiguous_range<TContainer>)
    /* implicit */ CMemoryBlock(const TContainer& _container)
        : m_pData(reinterpret_cast<const Byte*>(_container.data()))
        , m_size(NumericCast<SizeT>(_container.size() * sizeof(TContainer::value_type)))
    {
    }

    // C-style 배열
    template<typename T, std::size_t N>
    /* implicit */ CMemoryBlock(const T (&_array)[N])
        : m_pData(reinterpret_cast<const Byte*>(_array))
        , m_size(NumericCast<SizeT>(N * sizeof(T)))
    {
    }

    template<typename T>
        requires(!std::ranges::range<T>
                 && !std::is_array_v<T>
                 && std::is_standard_layout_v<T>
                 && !std::is_pointer_v<T>)
    /* implicit */ CMemoryBlock(const T& _data)
        : m_pData(reinterpret_cast<const Byte*>(&_data))
        , m_size(NumericCast<SizeT>(sizeof(T)))
    {
    }

    [[nodiscard]] const Byte*  GetPtr() const;
    [[nodiscard]] SizeT        GetByteWidth() const;
    [[nodiscard]] CMemoryBlock SubBlock(SizeT _offset) const;
    [[nodiscard]] CMemoryBlock SubBlock(SizeT _offset, SizeT _byteWidth) const;
    [[nodiscard]] bool         IsEmpty() const;

private:
    const Byte* m_pData = nullptr;
    SizeT       m_size  = 0;
};

}   // namespace Jam