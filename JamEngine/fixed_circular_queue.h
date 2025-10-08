#pragma once

namespace jam
{

enum class eFixedCircularQueueOverflowPolicy : Int8
{
    DiscardPush,   // 새로 들어오는 데이터를 버림
    PopFront,      // 가장 오래된 데이터를 제거하고 새 데이터를 추가
    ThrowError,    // 예외를 던짐
};

template<typename T, size_t Size, eFixedCircularQueueOverflowPolicy Policy = eFixedCircularQueueOverflowPolicy::ThrowError>
class fixed_ciruclar_queue
{
    static constexpr size_t k_bufferCapacity = Size + 1;

public:
    class iterator
    {
        friend class fixed_ciruclar_queue<T, Size, Policy>;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        iterator()
        {
        }

        iterator(const pointer _pCur, const pointer _pBufBegin, const pointer _pBufEnd)
            : m_pCur(_pCur)
            , m_pBufBegin(_pBufBegin)
            , m_pBufEnd(_pBufEnd)
        {
        }

        reference operator*() const
        {
            return *m_pCur;
        }

        pointer operator->() const
        {
            return m_pCur;
        }

        iterator& operator++()
        {
            if (++m_pCur == m_pBufEnd) m_pCur = m_pBufBegin;
            return *this;
        }
        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator& operator--()
        {
            if (m_pCur == m_pBufBegin) m_pCur = m_pBufEnd;
            --m_pCur;
            return *this;
        }

        iterator operator--(int)
        {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }

        iterator& operator+=(difference_type n)
        {
            difference_type offset      = (m_pCur - m_pBufBegin) + n;
            difference_type buffer_size = m_pBufEnd - m_pBufBegin;
            m_pCur                      = m_pBufBegin + (offset % buffer_size + buffer_size) % buffer_size;
            return *this;
        }

        iterator operator+(difference_type n) const
        {
            iterator tmp = *this;
            return tmp += n;
        }

        iterator& operator-=(const difference_type n)
        {
            return *this += -n;
        }

        iterator operator-(difference_type n) const
        {
            iterator tmp = *this;
            return tmp -= n;
        }

        difference_type operator-(const iterator& other) const
        {
            ptrdiff_t diff = m_pCur - other.m_pCur;
            if (diff < 0)
            {
                if (other.m_pCur < m_pCur + (m_pBufEnd - m_pBufBegin))
                {
                    diff += (m_pBufEnd - m_pBufBegin);
                }
            }
            return diff;
        }

        bool operator==(const iterator& other) const
        {
            return m_pCur == other.m_pCur;
        }

        bool operator!=(const iterator& other) const
        {
            return m_pCur != other.m_pCur;
        }

    private:
        pointer m_pCur      = nullptr;
        pointer m_pBufBegin = nullptr;
        pointer m_pBufEnd   = nullptr;
    };

    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using const_iterator  = iterator;
    using size_type       = size_t;

    fixed_ciruclar_queue()
        : m_data {}
        , m_begin(m_data, m_data, m_data + k_bufferCapacity)
        , m_end(m_data, m_data, m_data + k_bufferCapacity)
        , m_size(0)
    {
    }

    template<typename... Args>
    void emplace_back(Args&&... args)
    {
        if (full())
        {
            if constexpr (Policy == eFixedCircularQueueOverflowPolicy::ThrowError)
            {
                throw std::runtime_error("fixed_circular_queue overflow: cannot emplace_back to a full queue.");
            }
            else if constexpr (Policy == eFixedCircularQueueOverflowPolicy::DiscardPush)
            {
                return;
            }
            else if constexpr (Policy == eFixedCircularQueueOverflowPolicy::PopFront)
            {
                pop_front();
            }
        }
        std::construct_at(m_end.m_pCur, std::forward<Args>(args)...);
        ++m_end;
        ++m_size;
    }

    void push_back(const T& value)
    {
        if (full())
        {
            if constexpr (Policy == eFixedCircularQueueOverflowPolicy::ThrowError)
            {
                throw std::runtime_error("fixed_circular_queue overflow: cannot push_back to a full queue.");
            }
            else if constexpr (Policy == eFixedCircularQueueOverflowPolicy::DiscardPush)
            {
                return;
            }
            else if constexpr (Policy == eFixedCircularQueueOverflowPolicy::PopFront)
            {
                pop_front();
            }
        }

        *m_end = value;
        ++m_end;
        ++m_size;
    }

    void push_back(T&& value)
    {
        if (full())
        {
            if constexpr (Policy == eFixedCircularQueueOverflowPolicy::ThrowError)
            {
                throw std::runtime_error("fixed_circular_queue overflow: cannot push_back to a full queue.");
            }
            else if constexpr (Policy == eFixedCircularQueueOverflowPolicy::DiscardPush)
            {
                return;
            }
            else if constexpr (Policy == eFixedCircularQueueOverflowPolicy::PopFront)
            {
                pop_front();
            }
        }
        *m_end = std::move(value);
        ++m_end;
        ++m_size;
    }

    void pop_front()
    {
        if (!empty())
        {
            std::destroy_at(m_begin.m_pCur);
            ++m_begin;
            --m_size;
        }
        else
        {
            throw std::runtime_error("fixed_circular_queue underflow: cannot pop_front from an empty queue.");
        }
    }

    void clear()
    {
        while (!empty())
        {
            pop_front();
        }
    }

    reference front()
    {
        return *m_begin;
    }

    const_reference front() const
    {
        return *m_begin;
    }

    reference back()
    {
        iterator tmp = m_end;
        --tmp;
        return *tmp;
    }

    const_reference back() const
    {
        iterator tmp = m_end;
        --tmp;
        return *tmp;
    }

    JAM_NODISCARD size_type           size() const { return m_size; }
    JAM_NODISCARD bool                empty() const { return m_size == 0; }
    JAM_NODISCARD constexpr size_type capacity() const { return Size; }
    JAM_NODISCARD bool                full() const { return m_size == Size; }

    JAM_NODISCARD iterator       begin() { return m_begin; }
    JAM_NODISCARD iterator       end() { return m_end; }
    JAM_NODISCARD const_iterator begin() const { return m_begin; }
    JAM_NODISCARD const_iterator end() const { return m_end; }
    JAM_NODISCARD const_iterator cbegin() const { return m_begin; }
    JAM_NODISCARD const_iterator cend() const { return m_end; }

private:
    T         m_data[k_bufferCapacity];
    iterator  m_begin;
    iterator  m_end;
    size_type m_size;
};

}   // namespace jam