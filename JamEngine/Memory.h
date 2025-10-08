#pragma once

namespace jam
{

/// 메모리에 대한 단일 뷰
class MemoryView
{
public:
    MemoryView()
    {
    }

    MemoryView(const void* _pData, const size_t _byteWidth)
        : pData(static_cast<const Byte*>(_pData))
        , byteWidth(_byteWidth)
    {
        JAM_ASSERT(pData != nullptr, "메모리 뷰의 데이터 포인터는 nullptr일 수 없습니다.");
        JAM_ASSERT(byteWidth > 0, "메모리 뷰의 바이트 크기는 0보다 커야 합니다.");
    }

    /// 배열이 아닌 단일 POD 타입을 처리
    template<typename T>
        requires IsPlainOfData<T> && !IsStdContiguousSequenceContainer<T>
                 explicit MemoryView(const T& _value)
        : pData(reinterpret_cast<const Byte*>(&_value))
    , byteWidth(sizeof(T))
    {
        JAM_ASSERT(pData != nullptr, "메모리 뷰의 데이터 포인터는 nullptr일 수 없습니다.");
        JAM_ASSERT(byteWidth > 0, "메모리 뷰의 바이트 크기는 0보다 커야 합니다.");
    }

    /// std::vector, std::span, std::array, C-style 등 연속적인 메모리 블록을 가지는 표준 컨테이너
    template<typename ContainerT>
        requires IsStdContiguousSequenceContainer<ContainerT>
    explicit MemoryView(const ContainerT& _container)
        : pData(reinterpret_cast<const Byte*>(_container.data()))
        , byteWidth(_container.size() * sizeof(ContainerT::value_type))
    {
        JAM_ASSERT(pData != nullptr, "메모리 뷰의 데이터 포인터는 nullptr일 수 없습니다.");
        JAM_ASSERT(byteWidth > 0, "메모리 뷰의 바이트 크기는 0보다 커야 합니다.");
    }

    /// C 스타일 배열
    template<typename T, size_t N>
    explicit MemoryView(const T (&_array)[N])
        : pData(reinterpret_cast<const Byte*>(_array))
        , byteWidth(N * sizeof(T))
    {
        JAM_ASSERT(pData != nullptr, "메모리 뷰의 데이터 포인터는 nullptr일 수 없습니다.");
        JAM_ASSERT(byteWidth > 0, "메모리 뷰의 바이트 크기는 0보다 커야 합니다.");
    }

    JAM_NODISCARD const Byte* GetDataPtr() const
    {
        return pData;
    }

    JAM_NODISCARD size_t GetByteWidth() const
    {
        return byteWidth;
    }

    JAM_NODISCARD bool IsValid() const
    {
        return pData != nullptr && byteWidth > 0;
    }

private:
    const Byte* pData     = nullptr;
    size_t      byteWidth = 0;
};


}   // namespace jam