#pragma once
#include "Allocator.h"

namespace Jam::Tiny
{

template<typename T>
class RingBuffer
{
public:
    using value_type      = T;
    using size_type       = SizeT;
    using difference_type = std::ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;

    template<bool k_bIsConst>
    class IteratorBase
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<k_bIsConst, const T*, T*>;
        using reference         = std::conditional_t<k_bIsConst, const T&, T&>;

        IteratorBase() = default;
        IteratorBase(T* _pBegin, T* _pEnd, T* _pTail, T* _pCur)
            : m_pBegin(_pBegin)
            , m_pEnd(_pEnd)
            , m_pTail(_pTail)
            , m_pCur(_pCur)
        {
        }

        template<bool k_bOtherConst, typename = std::enable_if_t<k_bIsConst && !k_bOtherConst>>
        /* inplicit */ IteratorBase(const IteratorBase<k_bOtherConst>& _other)
            : m_pBegin(_other.m_pBegin)
            , m_pEnd(_other.m_pEnd)
            , m_pTail(_other.m_pTail)
            , m_pCur(_other.m_pCur)
        {
        }

        IteratorBase& operator++()
        {
            JAM_ASSERT(m_pCur != m_pTail, "Iterator out of bound.");
            m_pCur = (m_pCur + 1 == m_pEnd) ? m_pBegin : m_pCur + 1;
            return *this;
        }

        IteratorBase operator++(int)
        {
            IteratorBase tmp = *this;
            ++(*this);
            return tmp;
        }

        [[nodiscard]] bool operator==(const IteratorBase& _other) const
        {
            return m_pCur == _other.m_pCur;
        }

        [[nodiscard]] bool operator!=(const IteratorBase& _other) const
        {
            return !(*this == _other);
        }

        [[nodiscard]] std::conditional_t<k_bIsConst, const T&, T&> operator*() const
        {
            JAM_ASSERT(m_pCur != m_pTail, "Iterator out of bound.");
            return *m_pCur;
        }

        [[nodiscard]] pointer operator->() const
        {
            JAM_ASSERT(m_pCur != m_pTail, "Iterator out of bound.");
            return m_pCur;
        }

    private:
        T* m_pBegin = nullptr;   // mem begin
        T* m_pEnd   = nullptr;   // mem end
        T* m_pTail  = nullptr;   // logical end
        T* m_pCur   = nullptr;
    };

    using Iterator      = IteratorBase<false>;
    using ConstIterator = IteratorBase<true>;

    RingBuffer() = default;
    ~RingBuffer()
    {
        CleanUp_();
    }

    RingBuffer(const RingBuffer& _other)
    {
        m_begin      = 0;
        m_size       = _other.m_size;
        m_logicalCap = _other.m_logicalCap;
        if (m_logicalCap > 0)
        {
            m_ptr = static_cast<T*>(Malloc(sizeof(T) * (m_logicalCap + 1)));
            for (SizeT i = 0; i < m_size; ++i)
            {
                std::construct_at(&m_ptr[i], _other.m_ptr[_other.GetIndex_(i)]);
            }
        }
    }

    RingBuffer(RingBuffer&& _other) noexcept
    {
        m_ptr        = _other.m_ptr;
        m_begin      = _other.m_begin;
        m_size       = _other.m_size;
        m_logicalCap = _other.m_logicalCap;

        _other.m_ptr        = nullptr;
        _other.m_size       = 0;
        _other.m_logicalCap = 0;
        _other.m_begin      = 0;
    }

    RingBuffer& operator=(const RingBuffer& _other)
    {
        if (this != &_other)
        {
            CleanUp_();
            m_begin      = 0;
            m_size       = _other.m_size;
            m_logicalCap = _other.m_logicalCap;
            if (m_logicalCap > 0)
            {
                m_ptr = static_cast<T*>(Malloc(sizeof(T) * (m_logicalCap + 1)));
                for (SizeT i = 0; i < m_size; ++i)
                {
                    std::construct_at(&m_ptr[i], _other.m_ptr[_other.GetIndex_(i)]);
                }
            }
        }
        return *this;
    }

    RingBuffer& operator=(RingBuffer&& _other) noexcept
    {
        if (this != &_other)
        {
            CleanUp_();
            m_ptr        = _other.m_ptr;
            m_begin      = _other.m_begin;
            m_size       = _other.m_size;
            m_logicalCap = _other.m_logicalCap;

            _other.m_ptr        = nullptr;
            _other.m_size       = 0;
            _other.m_logicalCap = 0;
            _other.m_begin      = 0;
        }
        return *this;
    }

    [[nodiscard]] Iterator Begin()
    {
        return Iterator(m_ptr, m_ptr + m_logicalCap + 1, Tail_(), Head_());
    }

    [[nodiscard]] ConstIterator Begin() const
    {
        return ConstIterator(m_ptr, m_ptr + m_logicalCap + 1, Tail_(), Head_());
    }

    [[nodiscard]] Iterator End()
    {
        return Iterator(m_ptr, m_ptr + m_logicalCap + 1, Tail_(), Tail_());
    }

    [[nodiscard]] ConstIterator End() const
    {
        return ConstIterator(m_ptr, m_ptr + m_logicalCap + 1, Tail_(), Tail_());
    }

    template<typename U>
    void PushBack(U&& _value)
    {
        if (IsFull_())
        {
            Grow_(NextCap_(m_logicalCap));
        }

        std::construct_at(&m_ptr[GetIndex_(m_size)], std::forward<U>(_value));
        ++m_size;
    }

    template<typename... TArgs>
    void EmplaceBack(TArgs&&... _args)
    {
        if (IsFull_())
        {
            Grow_(NextCap_(m_logicalCap));
        }

        std::construct_at(&m_ptr[GetIndex_(m_size)], std::forward<TArgs>(_args)...);
        ++m_size;
    }

    template<typename U>
    void PushFront(U&& _value)
    {
        if (IsFull_())
        {
            Grow_(NextCap_(m_logicalCap));
        }

        m_begin = GetIndex_(-1);
        std::construct_at(&m_ptr[m_begin], std::forward<U>(_value));
        ++m_size;
    }

    template<typename... TArgs>
    void EmplaceFront(TArgs&&... _args)
    {
        if (IsFull_())
        {
            Grow_(NextCap_(m_logicalCap));
        }

        m_begin = GetIndex_(-1);
        std::construct_at(&m_ptr[m_begin], std::forward<TArgs>(_args)...);
        ++m_size;
    }

    void PopFront()
    {
        JAM_ASSERT(m_size > 0, "RingBuffer is empty.");
        std::destroy_at(&m_ptr[m_begin]);
        m_begin = GetIndex_(1);
        --m_size;
    }

    void PopBack()
    {
        JAM_ASSERT(m_size > 0, "RingBuffer is empty.");
        std::destroy_at(&m_ptr[GetIndex_(m_size - 1)]);
        --m_size;
    }

    [[nodiscard]] const T& operator[](const SizeT _idx) const
    {
        JAM_ASSERT(_idx >= 0 && _idx < m_size, "Index out of bound.");
        return m_ptr[GetIndex_(_idx)];
    }

    [[nodiscard]] T& operator[](const SizeT _idx)
    {
        JAM_ASSERT(_idx >= 0 && _idx < m_size, "Index out of bound.");
        return m_ptr[GetIndex_(_idx)];
    }

    [[nodiscard]] T& Back()
    {
        JAM_ASSERT(m_size > 0, "RingBuffer is empty.");
        return m_ptr[GetIndex_(m_size - 1)];
    }

    [[nodiscard]] const T& Back() const
    {
        JAM_ASSERT(m_size > 0, "RingBuffer is empty.");
        return m_ptr[GetIndex_(m_size - 1)];
    }

    [[nodiscard]] T& Front()
    {
        JAM_ASSERT(m_size > 0, "RingBuffer is empty.");
        return m_ptr[m_begin];
    }

    [[nodiscard]] const T& Front() const
    {
        JAM_ASSERT(m_size > 0, "RingBuffer is empty.");
        return m_ptr[m_begin];
    }

    [[nodiscard]] SizeT GetSize() const
    {
        return m_size;
    }

    [[nodiscard]] SizeT GetCapacity() const
    {
        return m_logicalCap;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return m_size == 0;
    }

    void Clear()
    {
        for (SizeT i = 0; i < m_size; ++i)
        {
            std::destroy_at(&m_ptr[GetIndex_(i)]);
        }
        m_size  = 0;
        m_begin = 0;
    }

    void Reserve(const SizeT _newLogicalCap)
    {
        if (_newLogicalCap > m_logicalCap)
        {
            Grow_(_newLogicalCap);
        }
    }

private:
    void CleanUp_()
    {
        if (m_ptr)
        {
            for (SizeT i = 0; i < m_size; ++i)
            {
                std::destroy_at(&m_ptr[GetIndex_(i)]);
            }
            Free(m_ptr);
            m_ptr = nullptr;
        }
        m_size       = 0;
        m_logicalCap = 0;
        m_begin      = 0;
    }

    void Grow_(const SizeT _newCap)   // _newCap is logical capacity, physical capacity is _newCap + 1
    {
        const SizeT cap  = _newCap + 1;
        T*          pNew = static_cast<T*>(Malloc(sizeof(T) * cap));

        if (m_ptr)
        {
            for (SizeT i = 0; i < m_size; ++i)
            {
                const SizeT idx = GetIndex_(i);
                std::construct_at(&pNew[i], std::move(m_ptr[idx]));
                std::destroy_at(&m_ptr[idx]);
            }

            Free(m_ptr);
        }

        m_ptr        = pNew;
        m_begin      = 0;
        m_logicalCap = _newCap;
    }

    // _idx is logical index, return physical index
    [[nodiscard]] SizeT GetIndex_(const SizeT _idx) const
    {
        JAM_ASSERT(_idx >= -1 && _idx <= m_size, "Index out of bound."); 
        return (m_begin + _idx + (m_logicalCap + 1)) % (m_logicalCap + 1);
    }

    [[nodiscard]] bool IsFull_() const
    {
        return m_size >= m_logicalCap;
    }

    [[nodiscard]] SizeT NextCap_(const SizeT _curCap) const
    {
        return _curCap == 0 ? 4 : _curCap * 2;
    }

    [[nodiscard]] T* Head_()
    {
        return m_ptr ? m_ptr + m_begin : nullptr;
    }

    [[nodiscard]] T* Tail_()
    {
        return m_ptr ? m_ptr + ((m_begin + m_size) % (m_logicalCap + 1)) : nullptr;
    }

    [[nodiscard]] const T* Head_() const
    {
        return m_ptr ? m_ptr + m_begin : nullptr;
    }

    [[nodiscard]] const T* Tail_() const
    {
        return m_ptr ? m_ptr + ((m_begin + m_size) % (m_logicalCap + 1)) : nullptr;
    }

    T*    m_ptr        = nullptr;
    SizeT m_begin      = 0;
    SizeT m_size       = 0;
    SizeT m_logicalCap = 0;
};

}   // namespace Jam::Tiny