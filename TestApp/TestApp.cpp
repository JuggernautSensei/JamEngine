#include <cassert>
#include <iostream>

template<typename T, size_t N>
class power_of_2_circular_q
{
    static_assert((N & (N - 1)) == 0, "N must be a power of 2.");
    constexpr static size_t k_mask = N - 1;

    template<bool IsConst>
    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = std::conditional_t<IsConst, const T*, T*>;
        using reference         = std::conditional_t<IsConst, const T&, T&>;
        using ContainerPointer  = std::conditional_t<IsConst, const power_of_2_circular_q*, power_of_2_circular_q*>;

        Iterator(ContainerPointer buf, const size_t pos)
            : m_buf(buf)
            , m_pos(pos)
        {
        }

        reference operator*() const noexcept
        {
            return (*m_buf)[m_pos];
        }

        pointer operator->() const noexcept
        {
            return &(*m_buf)[m_pos];
        }

        Iterator& operator++() noexcept
        {
            ++m_pos;
            return *this;
        }

        Iterator operator++(int) noexcept
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) noexcept
        {
            return a.m_pos == b.m_pos && a.m_buf == b.m_buf;
        }

        friend bool operator!=(const Iterator& a, const Iterator& b) noexcept
        {
            return !(a == b);
        }

    private:
        ContainerPointer m_buf = nullptr;
        size_t           m_pos = 0;
    };

public:
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    template<typename U>
        requires std::is_convertible_v<U, T>
    void push(T&& _value)
    {
        assert(!full() && "buffer is full. Cannot push_back.");
        m_items[m_rear] = std::forward<U>(_value);
        m_rear          = (m_rear + 1) & k_mask;
    }

    void pop() noexcept
    {
        assert(!empty() && "buffer is empty. Cannot pop.");
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            std::destroy_at(&m_items[m_front]);
        }
        m_front = (m_front + 1) & k_mask;
    }

    void clear() noexcept
    {
        while (!empty())
        {
            pop();
        }
        m_front = 0;
        m_rear  = 0;
    }

    [[nodiscard]] T& front() noexcept
    {
        assert(!empty() && "buffer is empty.");
        return m_items[m_front];
    }

    [[nodiscard]] const T& front() const noexcept
    {
        assert(!empty() && "buffer is empty.");
        return m_items[m_front];
    }

    [[nodiscard]] T& back() noexcept
    {
        assert(!empty() && "buffer is empty.");
        return m_items[(m_rear - 1) & k_mask];
    }

    [[nodiscard]] const T& back() const noexcept
    {
        assert(!empty() && "buffer is empty.");
        return m_items[(m_rear - 1) & k_mask];
    }

    [[nodiscard]] T& at(const size_t _index) noexcept
    {
        assert(_index < size() && "index out of range.");
        return m_items[(m_front + _index) & k_mask];
    }

    [[nodiscard]] const T& at(const size_t _index) const noexcept
    {
        assert(_index < size() && "index out of range.");
        return m_items[(m_front + _index) & k_mask];
    }

    [[nodiscard]] T& operator[](size_t _index) noexcept
    {
        assert(_index < size() && "index out of range.");
        return m_items[(m_front + _index) & k_mask];
    }

    [[nodiscard]] const T& operator[](size_t _index) const noexcept
    {
        assert(_index < size() && "index out of range.");
        return m_items[(m_front + _index) & k_mask];
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_front == m_rear;
    }

    [[nodiscard]] bool full() const noexcept
    {
        return ((m_rear + 1) & k_mask) == m_front;
    }

    [[nodiscard]] size_t size() const noexcept
    {
        return (m_rear + N - m_front) & k_mask;
    }

    [[nodiscard]] static constexpr size_t capacity() noexcept
    {
        return N;
    }

    [[nodiscard]] static constexpr size_t max_size() noexcept
    {
        return N;
    }

    [[nodiscard]] iterator begin() noexcept
    {
        return iterator(this, 0);
    }

    [[nodiscard]] iterator end() noexcept
    {
        return iterator(this, size());
    }

    [[nodiscard]] const_iterator begin() const noexcept
    {
        return const_iterator(this, 0);
    }

    [[nodiscard]] const_iterator end() const noexcept
    {
        return const_iterator(this, size());
    }

    [[nodiscard]] const_iterator cbegin() const noexcept
    {
        return const_iterator(this, 0);
    }

    [[nodiscard]] const_iterator cend() const noexcept
    {
        return const_iterator(this, size());
    }
 
private:
    T      m_items[N] = {};
    size_t m_front    = 0;
    size_t m_rear     = 0;
};