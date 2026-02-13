#pragma once

namespace Jam
{

constexpr UInt32 k_invalidHandleIdx = 0x00FF'FFFF;

struct InvalidHandleType
{
    struct Tag
    {
    };

    constexpr explicit InvalidHandleType(Tag) noexcept
    {
    }
};

constexpr InvalidHandleType k_invalidHandle = InvalidHandleType { InvalidHandleType::Tag {} };

/// @brief A stage-safe handle for a resource.
/// @tparam T The resource stage this handle references.
template<typename T>
struct Handle
{
    Handle()
        : index(k_invalidHandleIdx)
        , token(0)
    {
    }

    /* implicit */ Handle(const InvalidHandleType) noexcept
        : index(k_invalidHandleIdx)
        , token(0)
    {
    }

    Handle(const UInt32 _index, const UInt32 _token) noexcept
    {
        index = _index;
        token = _token;
    }

    Handle& operator=(const InvalidHandleType) noexcept
    {
        index = k_invalidHandleIdx;
        token = 0;
        return *this;
    }

    [[nodiscard]] bool operator==(const Handle& other) const noexcept
    {
        return value == other.value;
    }

    [[nodiscard]] bool operator!=(const Handle& other) const noexcept
    {
        return !(*this == other);
    }

    [[nodiscard]] bool IsValid() const noexcept
    {
        return index != k_invalidHandleIdx;
    }

    /* implicit */  operator bool() const noexcept
    {
        return IsValid();
    }

    union
    {
        UInt32 value;
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
        : index(k_invalidHandleIdx)
        , token(0)
    {
    }

    /* implicit */ AnyHandle(const InvalidHandleType) noexcept
        : index(k_invalidHandleIdx)
        , token(0)
    {
    }

    template<typename T>
    /* implicit */ AnyHandle(const Handle<T> handle) noexcept
        : AnyHandle(handle.index, handle.token)
    {
    }

    AnyHandle(const UInt32 _index, const UInt32 _token) noexcept
        : index(_index)
        , token(_token)
    {
    }

    AnyHandle& operator=(const InvalidHandleType) noexcept
    {
        index = k_invalidHandleIdx;
        token = 0;
        return *this;
    }

    [[nodiscard]] bool operator==(const AnyHandle& other) const noexcept
    {
        return value == other.value;
    }

    [[nodiscard]] bool operator!=(const AnyHandle& other) const noexcept
    {
        return !(*this == other);
    }

    union
    {
        UInt32 value;
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
        UInt32 nextFreeIndex = k_invalidHandleIdx;
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
            , m_endIndex(NumericCast<UInt32>(_pAllocator->m_slots.size()))
        {
            Advance_();
        }

        Iterator& operator++() noexcept
        {
            ++m_index;
            Advance_();
            return *this;
        }

        [[nodiscard]] HandleT operator*() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator out of bounds");
            return HandleT { m_index, m_pAllocator->m_slots[m_index].token };
        }

        [[nodiscard]] HandleT GetHandle() const noexcept
        {
            JAM_ASSERT(IsValid_(), "Iterator out of bounds");
            return **this;
        }

        [[nodiscard]] bool operator==(const Iterator& other) const noexcept
        {
            return m_pAllocator == other.m_pAllocator && m_index == other.m_index;
        }

        [[nodiscard]] bool operator!=(const Iterator& other) const noexcept
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

        [[nodiscard]] bool IsValid_() const noexcept
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

    [[nodiscard]] HandleT Alloc()
    {
        UInt32 index;
        if (m_freeSlotIndex != k_invalidHandleIdx)
        {
            index           = m_freeSlotIndex;
            m_freeSlotIndex = m_slots[index].nextFreeIndex;
        }
        else
        {
            index = NumericCast<UInt32>(m_slots.size());
            m_slots.emplace_back();
        }

        ++m_numAlloced;
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
        --m_numAlloced;
    }

    [[nodiscard]] bool IsValid(const HandleT handle) const noexcept
    {
        const Slot& slot = m_slots[handle.index];
        return CmpLess(handle.index, m_slots.size())
            && slot.token == handle.token
            && slot.bAlloced;
    }

    void Clear() noexcept
    {
        m_slots.clear();
        m_freeSlotIndex = k_invalidHandleIdx;
        m_numAlloced    = 0;
    }

    [[nodiscard]] UInt32 GetAllocedCount() const noexcept
    {
        return m_numAlloced;
    }

    [[nodiscard]] IteratorT begin() noexcept
    {
        return IteratorT(this);
    }

    [[nodiscard]] IteratorT end() noexcept
    {
        return IteratorT(this, NumericCast<UInt32>(m_slots.size()));
    }

    [[nodiscard]] ConstIteratorT begin() const noexcept
    {
        return ConstIteratorT(this);
    }

    [[nodiscard]] ConstIteratorT end() const noexcept
    {
        return ConstIteratorT(this, NumericCast<UInt32>(m_slots.size()));
    }

    [[nodiscard]] ConstIteratorT cbegin() const noexcept
    {
        return ConstIteratorT(this);
    }

    [[nodiscard]] ConstIteratorT cend() const noexcept
    {
        return ConstIteratorT(this, NumericCast<UInt32>(m_slots.size()));
    }

private:
    Vector<Slot> m_slots         = {};
    UInt32       m_freeSlotIndex = k_invalidHandleIdx;
    Int32        m_numAlloced    = 0;
};

}   // namespace Jam