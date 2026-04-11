#pragma once

namespace Jam
{

static constexpr UInt32 k_nullHandleValue  = 0xFFFF'FFFF;
static constexpr UInt32 k_handleIndexMask  = 0x00FF'FFFF;
static constexpr UInt32 k_handleTokenMask  = 0xFF00'0000;
static constexpr UInt32 k_handleTokenShift = 24;

struct NullHandleType
{
    struct Tag
    {
    };

    constexpr explicit NullHandleType(Tag) noexcept
    {
    }
};

constexpr NullHandleType k_nullHandle = NullHandleType { NullHandleType::Tag {} };

template<typename T>
class Handle
{
public:
    Handle()
    {
    }

    /* implicit */ Handle(const NullHandleType) noexcept
        : m_value(k_nullHandleValue)
    {
    }

    Handle(const UInt32 _index, const UInt32 _token) noexcept
        : m_value((_index & k_handleIndexMask) | ((_token << k_handleTokenShift) & k_handleTokenMask))
    {
    }

    /* implicit */ Handle(const UInt32 _value) noexcept
        : m_value(_value)
    {
    }

    Handle& operator=(const NullHandleType) noexcept
    {
        m_value = k_nullHandleValue;
        return *this;
    }

    [[nodiscard]] bool operator==(const Handle& other) const noexcept
    {
        return m_value == other.m_value;
    }

    [[nodiscard]] bool operator!=(const Handle& other) const noexcept
    {
        return !(*this == other);
    }

    [[nodiscard]] bool IsValid() const noexcept
    {
        return m_value != k_nullHandleValue;
    }

    explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] UInt32 GetIndex() const noexcept
    {
        return m_value & k_handleIndexMask;
    }

    [[nodiscard]] UInt32 GetToken() const noexcept
    {
        return (m_value & k_handleTokenMask) >> k_handleTokenShift;
    }

    [[nodiscard]] UInt32 GetValue() const noexcept
    {
        return m_value;
    }

private:
    UInt32 m_value = k_nullHandleValue;
};

template<typename T>
class HandleAllocator
{
    struct Slot
    {
        UInt32 nextFreeIndex = k_nullHandleValue;
        UInt8  token         = 0;
        bool   bAlloced      = false;
    };

public:
    using HandleT = Handle<T>;

    template<bool IsConst>
    class IteratorBase
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = Handle<T>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        using AllocatorT = std::conditional_t<IsConst, const HandleAllocator, HandleAllocator>;
        using HandleT    = Handle<T>;

        IteratorBase()
        {
        }

        explicit IteratorBase(AllocatorT* _pAllocator) noexcept
            : IteratorBase(_pAllocator, 0)
        {
        }

        explicit IteratorBase(AllocatorT* _pAllocator, const UInt32 _startIndex) noexcept
            : m_pAllocator(_pAllocator)
            , m_index(_startIndex)
            , m_endIndex(NarrowCast<UInt32>(_pAllocator->m_slots.size()))
        {
            Advance_();
        }

        IteratorBase& operator++() noexcept
        {
            ++m_index;
            Advance_();
            return *this;
        }

        [[nodiscard]] HandleT operator*() const noexcept
        {
            JAM_ASSERT(IsValid_(), "IteratorBase out of bounds");
            return HandleT { m_index, m_pAllocator->m_slots[m_index].token };
        }

        [[nodiscard]] HandleT GetHandle() const noexcept
        {
            JAM_ASSERT(IsValid_(), "IteratorBase out of bounds");
            return **this;
        }

        [[nodiscard]] bool operator==(const IteratorBase& other) const noexcept
        {
            return m_pAllocator == other.m_pAllocator && m_index == other.m_index;
        }

        [[nodiscard]] bool operator!=(const IteratorBase& other) const noexcept
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

    using Iterator      = IteratorBase<false>;
    using ConstIterator = IteratorBase<true>;

    HandleAllocator()  = default;
    ~HandleAllocator() = default;

    HandleAllocator(const HandleAllocator&)            = delete;
    HandleAllocator& operator=(const HandleAllocator&) = delete;
    HandleAllocator(HandleAllocator&&)                 = default;
    HandleAllocator& operator=(HandleAllocator&&)      = default;

    [[nodiscard]] HandleT Alloc()
    {
        UInt32 index = 0;
        if (m_freeSlotIndex != k_nullHandleValue)
        {
            index           = m_freeSlotIndex;
            m_freeSlotIndex = m_slots[index].nextFreeIndex;
        }
        else
        {
            index = NarrowCast<UInt32>(m_slots.size());
            m_slots.emplace_back();
        }

        ++m_numAlloced;
        m_slots[index].bAlloced = true;
        return HandleT(index, m_slots[index].token);
    }

    void Free(const HandleT handle) noexcept
    {
        JAM_ASSERT(IsValid(handle), "Invalid fbh");
        UInt32 index = handle.GetIndex();
        ++m_slots[index].token;   // 오버플로우는 오류가 아님
        m_slots[index].bAlloced      = false;
        m_slots[index].nextFreeIndex = m_freeSlotIndex;
        m_freeSlotIndex              = index;
        --m_numAlloced;
    }

    [[nodiscard]] bool IsValid(const HandleT handle) const noexcept
    {
        const UInt32 index = handle.GetIndex();
        const Slot&  slot  = m_slots[index];
        return std::cmp_less(index, m_slots.size())
            && slot.token == handle.GetToken()
            && slot.bAlloced;
    }

    void Clear() noexcept
    {
        m_slots.clear();
        m_freeSlotIndex = k_nullHandleValue;
        m_numAlloced    = 0;
    }

    [[nodiscard]] UInt32 GetAllocedCount() const noexcept
    {
        return m_numAlloced;
    }

    [[nodiscard]] Iterator begin() noexcept
    {
        return Iterator(this);
    }

    [[nodiscard]] Iterator end() noexcept
    {
        return Iterator(this, NarrowCast<UInt32>(m_slots.size()));
    }

    [[nodiscard]] ConstIterator begin() const noexcept
    {
        return ConstIterator(this);
    }

    [[nodiscard]] ConstIterator end() const noexcept
    {
        return ConstIterator(this, NarrowCast<UInt32>(m_slots.size()));
    }

    [[nodiscard]] ConstIterator cbegin() const noexcept
    {
        return ConstIterator(this);
    }

    [[nodiscard]] ConstIterator cend() const noexcept
    {
        return ConstIterator(this, NarrowCast<UInt32>(m_slots.size()));
    }

private:
    Vector<Slot> m_slots         = {};
    UInt32       m_freeSlotIndex = k_nullHandleValue;
    Int32        m_numAlloced    = 0;
};

}   // namespace Jam