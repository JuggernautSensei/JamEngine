#pragma once

namespace Jam
{

static constexpr UInt32 kInvalidHandleValue = 0xFFFF'FFFF;
static constexpr UInt32 kHandleIndexMask    = 0x00FF'FFFF;
static constexpr UInt32 kHandleTokenMask    = 0xFF00'0000;
static constexpr UInt32 kHandleTokenShift   = 24;

struct InvalidHandleType
{
    struct Tag
    {
    };

    constexpr explicit InvalidHandleType(Tag) noexcept
    {
    }
};

constexpr InvalidHandleType kInvalidHandle = InvalidHandleType { InvalidHandleType::Tag {} };

template<typename T>
class Handle
{
public:
    Handle()
    {
    }

    /* implicit */ Handle(const InvalidHandleType) noexcept
        : m_value(kInvalidHandleValue)
    {
    }

    Handle(const UInt32 _index, const UInt32 _token) noexcept
        : m_value((_index & kHandleIndexMask) | ((_token << kHandleTokenShift) & kHandleTokenMask))
    {
    }

    /* implicit */ Handle(const UInt32 _value) noexcept
        : m_value(_value)
    {
    }

    Handle& operator=(const InvalidHandleType) noexcept
    {
        m_value = kInvalidHandleValue;
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
        return m_value != kInvalidHandleValue;
    }

    explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] UInt32 GetIndex() const noexcept
    {
        return m_value & kHandleIndexMask;
    }

    [[nodiscard]] UInt32 GetToken() const noexcept
    {
        return (m_value & kHandleTokenMask) >> kHandleTokenShift;
    }

    [[nodiscard]] UInt32 GetValue() const noexcept
    {
        return m_value;
    }

private:
    UInt32 m_value = kInvalidHandleValue;
};

template<typename T>
class HandleAllocator
{
    struct Slot
    {
        UInt32 nextFreeIndex = kInvalidHandleValue;
        UInt8  token         = 0;
        bool   bAlloced      = false;
    };

public:
    using HandleT = Handle<T>;

public:
    class Iterator
    {
    public:
        Iterator() = default;

        explicit Iterator(HandleAllocator* _pAllocator, const UInt32 _startIndex) noexcept
            : m_pAllocator(_pAllocator)
            , m_index(_startIndex)
            , m_endIndex(static_cast<UInt32>(_pAllocator->m_slots.size()))
        {
            Next_();
        }

        Iterator& operator++() noexcept
        {
            ++m_index;
            Next_();
            return *this;
        }

        [[nodiscard]] HandleT operator*() const noexcept
        {
            JAM_ASSERT(m_pAllocator, "Iterator not initialized");
            JAM_ASSERT(m_index < m_endIndex, "Iterator out of bounds");

            const Slot slot = m_pAllocator->m_slots[m_index];
            JAM_ASSERT(slot.bAlloced, "Iterator points to a free slot. pos = {}, token = {}", m_index, slot.token);
            return { m_index, slot.token };
        }

        [[nodiscard]] bool operator==(const Iterator& other) const noexcept
        {
            return m_pAllocator == other.m_pAllocator && m_index == other.m_index;
        }

        [[nodiscard]] bool operator!=(const Iterator& other) const noexcept
        {
            return !(*this == other);
        }

    protected:
        void Next_() noexcept
        {
            JAM_ASSERT(m_pAllocator, "IteratorBase not initialized");
            JAM_ASSERT(m_index <= m_endIndex, "IteratorBase out of bounds");

            while (m_index < m_endIndex && m_pAllocator->m_slots[m_index].bAlloced == false)
            {
                ++m_index;
            }
        }

        HandleAllocator* m_pAllocator = nullptr;
        UInt32           m_index      = 0;
        UInt32           m_endIndex   = 0;
    };

    using ConstIterator = Iterator;

public:
    HandleAllocator()  = default;
    ~HandleAllocator() = default;

    HandleAllocator(const HandleAllocator&)            = delete;
    HandleAllocator& operator=(const HandleAllocator&) = delete;
    HandleAllocator(HandleAllocator&&)                 = default;
    HandleAllocator& operator=(HandleAllocator&&)      = default;

    [[nodiscard]] HandleT Alloc()
    {
        UInt32 index = 0;
        if (m_freeSlotIndex != kInvalidHandleValue)
        {
            index           = m_freeSlotIndex;
            m_freeSlotIndex = m_slots[index].nextFreeIndex;
        }
        else
        {
            index = static_cast<UInt32>(m_slots.size());
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
        const Slot slot = m_slots[handle.GetIndex()];
        return slot.token == handle.GetToken() && slot.bAlloced;
    }

    void Clear() noexcept
    {
        m_slots.clear();
        m_freeSlotIndex = kInvalidHandleValue;
        m_numAlloced    = 0;
    }

    [[nodiscard]] SizeT GetAllocedCount() const noexcept
    {
        return m_numAlloced;
    }

    [[nodiscard]] Iterator Begin() noexcept
    {
        return { this, 0 };
    }

    [[nodiscard]] Iterator End() noexcept
    {
        return { this, static_cast<UInt32>(m_slots.size()) };
    }

    [[nodiscard]] ConstIterator Begin() const noexcept
    {
        return { this, 0 };
    }

    [[nodiscard]] ConstIterator End() const noexcept
    {
        return { this, static_cast<UInt32>(m_slots.size()) };
    }

public:   // for STL
    using iterator       = Iterator;
    using const_iterator = ConstIterator;

    [[nodiscard]] iterator begin() noexcept
    {
        return Begin();
    }

    [[nodiscard]] iterator end() noexcept
    {
        return End();
    }

    [[nodiscard]] const_iterator begin() const noexcept
    {
        return Begin();
    }

    [[nodiscard]] const_iterator end() const noexcept
    {
        return End();
    }

private:
    Vector<Slot> m_slots         = {};
    UInt32       m_freeSlotIndex = kInvalidHandleValue;
    SizeT        m_numAlloced    = 0;
};

}   // namespace Jam