#pragma once

namespace jam
{

constexpr UInt32 k_invalidHandleIndex = 0x00FFFFFF;
constexpr UInt32 k_invalidHandleToken = 0;
constexpr UInt32 k_invalidHandleValue = 0x00FFFFFF;

struct InvalidHandleType
{
};
constexpr InvalidHandleType k_invalidHandle {};

/// @brief A stage-safe handle for a resource.
/// @tparam T The resource stage this handle references.
template<typename T>
struct Handle
{
    Handle()
    {
    }

    /* implicit */ Handle(const InvalidHandleType) noexcept
    {
    }

    Handle(const UInt32 _index, const UInt32 _token) noexcept
    {
        index = _index;
        token = _token;
    }

    explicit Handle(const UInt32 _value) noexcept
        : value(_value)
    {
    }

    Handle& operator=(const InvalidHandleType) noexcept
    {
        value = k_invalidHandleValue;
        return *this;
    }

    JAM_NODISCARD bool operator==(const Handle& other) const noexcept
    {
        return value == other.value;
    }

    JAM_NODISCARD bool operator!=(const Handle& other) const noexcept
    {
        return !(*this == other);
    }

    union
    {
        UInt32 value = k_invalidHandleValue;
        struct
        {
            UInt32 index : 24;
            UInt32 token : 8;
        };
    };
};

struct AnyHandle
{
    AnyHandle()
    {
    }

    /* implicit */ AnyHandle(const InvalidHandleType) noexcept
    {
    }

    template<typename T>
    /* implicit */ AnyHandle(const Handle<T> handle) noexcept
        : AnyHandle(handle.index, handle.token)
    {
    }

    AnyHandle(const UInt32 _index, const UInt32 _token) noexcept
    {
        index = _index;
        token = _token;
    }

    AnyHandle& operator=(const InvalidHandleType) noexcept
    {
        value = k_invalidHandleValue;
        return *this;
    }

    JAM_NODISCARD bool operator==(const AnyHandle& other) const noexcept
    {
        return value == other.value;
    }

    JAM_NODISCARD bool operator!=(const AnyHandle& other) const noexcept
    {
        return !(*this == other);
    }

    union
    {
        UInt32 value = k_invalidHandleValue;
        struct
        {
            UInt32 index : 24;
            UInt32 token : 8;
        };
    };
};

/// @brief 핸들 할당기
template<typename T>
class HandleAllocator
{
    struct Slot
    {
        UInt32 nextFreeIndex = k_invalidHandleIndex;
        UInt8  token         = 0;
        bool   bAlloced      = false;
    };

    template<bool IsConst>
    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = Handle<T>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        using AllocatorT = std::conditional_t<IsConst, const HandleAllocator, HandleAllocator>;
        using HandleT    = Handle<T>;

        Iterator()
        {
        }

        explicit Iterator(AllocatorT* _pAllocator) noexcept
            : Iterator(_pAllocator, 0)
        {
        }

        explicit Iterator(AllocatorT* _pAllocator, const UInt32 _startIndex) noexcept
            : m_pAllocator(_pAllocator)
            , m_index(_startIndex)
            , m_endIndex(static_cast<UInt32>(_pAllocator->m_slots.size()))
        {
            Advance_();
        }

        Iterator& operator++() noexcept
        {
            ++m_index;
            Advance_();
            return *this;
        }

        JAM_NODISCARD HandleT operator*() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator out of bounds");
            return HandleT { m_index, m_pAllocator->m_slots[m_index].token };
        }

        JAM_NODISCARD HandleT GetHandle() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator out of bounds");
            return **this;
        }

        JAM_NODISCARD bool operator==(const Iterator& other) const noexcept
        {
            return m_pAllocator == other.m_pAllocator && m_index == other.m_index;
        }

        JAM_NODISCARD bool operator!=(const Iterator& other) const noexcept
        {
            return !(*this == other);
        }

    private:
        void Advance_() noexcept
        {
            while (m_index < m_endIndex && m_pAllocator->m_slots[m_index].bAlloced == false)
            {
                ++m_index;
            }
        }

        JAM_NODISCARD bool IsValid_() const noexcept
        {
            return m_pAllocator && m_index < m_endIndex && m_pAllocator->m_slots[m_index].bAlloced;
        }

        AllocatorT* m_pAllocator = nullptr;
        UInt32      m_index      = 0;
        UInt32      m_endIndex   = 0;
    };

public:
    using HandleT        = Handle<T>;
    using IteratorT      = Iterator<false>;
    using ConstIteratorT = Iterator<true>;

    HandleAllocator()  = default;
    ~HandleAllocator() = default;

    HandleAllocator(const HandleAllocator&)            = delete;
    HandleAllocator& operator=(const HandleAllocator&) = delete;
    HandleAllocator(HandleAllocator&&)                 = default;
    HandleAllocator& operator=(HandleAllocator&&)      = default;

    JAM_NODISCARD HandleT Alloc()
    {
        UInt32 index;
        if (m_freeSlotIndex != k_invalidHandleIndex)
        {
            index           = m_freeSlotIndex;
            m_freeSlotIndex = m_slots[index].nextFreeIndex;
        }
        else
        {
            index = static_cast<UInt32>(m_slots.size());
            m_slots.emplace_back();
        }

        ++m_allocedCount;
        m_slots[index].bAlloced = true;
        return HandleT(index, m_slots[index].token);
    }

    void Free(const HandleT handle) noexcept
    {
        JAM_ASSERT(IsValid(handle), "Invalid handle");
        UInt32 index = handle.index;
        ++m_slots[index].token;   // 오버플로우는 오휴가 아님
        m_slots[index].bAlloced      = false;
        m_slots[index].nextFreeIndex = m_freeSlotIndex;
        m_freeSlotIndex              = index;
        --m_allocedCount;
    }

    JAM_NODISCARD bool IsValid(const HandleT handle) const noexcept
    {
        UInt32 index = handle.index;
        return index < m_slots.size()
            && m_slots[index].token == handle.token
            && m_slots[index].bAlloced;
    }

    void Clear() noexcept
    {
        m_slots.clear();
        m_freeSlotIndex = k_invalidHandleIndex;
        m_allocedCount  = 0;
    }

    JAM_NODISCARD UInt32 GetAllocedCount() const noexcept
    {
        return m_allocedCount;
    }

    JAM_NODISCARD IteratorT begin() noexcept
    {
        return IteratorT(this);
    }

    JAM_NODISCARD IteratorT end() noexcept
    {
        return IteratorT(this, static_cast<UInt32>(m_slots.size()));
    }

    JAM_NODISCARD ConstIteratorT begin() const noexcept
    {
        return ConstIteratorT(this);
    }

    JAM_NODISCARD ConstIteratorT end() const noexcept
    {
        return ConstIteratorT(this, static_cast<UInt32>(m_slots.size()));
    }

    JAM_NODISCARD ConstIteratorT cbegin() const noexcept
    {
        return ConstIteratorT(this);
    }

    JAM_NODISCARD ConstIteratorT cend() const noexcept
    {
        return ConstIteratorT(this, static_cast<UInt32>(m_slots.size()));
    }

private:
    std::vector<Slot> m_slots         = {};
    UInt32            m_freeSlotIndex = k_invalidHandleIndex;
    Int32             m_allocedCount  = 0;
};

}   // namespace jam