#pragma once

namespace jam
{

template<typename T, size_t Size>
class fixed_vector
{
public:
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using iterator        = T*;
    using const_iterator  = const T*;

    fixed_vector()
        : m_size(0)
    {
    }

    template<typename U>
    void push_back(U&& value)
    {
        JAM_ASSERT(!full(), "fixed_vector is full. Cannot push_back.");
        m_data[m_size] = std::forward<U>(value);
        m_size++;
    }

    void pop_back()
    {
        JAM_ASSERT(!empty(), "fixed_vector is empty. Cannot pop_back.");
        std::destroy_at(&m_data[m_size]);
        m_size--;
    }

    void clear() noexcept
    {
        while (!empty())
        {
            pop_back();
        }
        m_size = 0;
    }

    reference operator[](size_type pos)
    {
        JAM_ASSERT(pos < m_size, "Index out of bounds in fixed_vector.");
        return m_data[pos];
    }

    const_reference operator[](size_type pos) const
    {
        JAM_ASSERT(pos < m_size, "Index out of bounds in fixed_vector.");
        return m_data[pos];
    }

    reference front()
    {
        JAM_ASSERT(!empty(), "fixed_vector is empty. Cannot access front.");
        return m_data[0];
    }

    const_reference front() const
    {
        JAM_ASSERT(!empty(), "fixed_vector is empty. Cannot access front.");
        return m_data[0];
    }

    reference back()
    {
        JAM_ASSERT(!empty(), "fixed_vector is empty. Cannot access front.");
        return m_data[m_size - 1];
    }

    const_reference back() const
    {
        JAM_ASSERT(!empty(), "fixed_vector is empty. Cannot access front.");
        return m_data[m_size - 1];
    }

    pointer       data() noexcept { return m_data; }
    const_pointer data() const noexcept { return m_data; }

    JAM_NODISCARD size_type           size() const { return m_size; }
    JAM_NODISCARD bool                empty() const { return m_size == 0; }
    JAM_NODISCARD constexpr size_type capacity() const { return Size; }
    JAM_NODISCARD bool                full() const { return m_size == Size; }

    JAM_NODISCARD iterator       begin() { return m_data; }
    JAM_NODISCARD iterator       end() { return m_data + m_size; }
    JAM_NODISCARD const_iterator begin() const { return m_data; }
    JAM_NODISCARD const_iterator end() const { return m_data + m_size; }
    JAM_NODISCARD const_iterator cbegin() const { return m_data; }
    JAM_NODISCARD const_iterator cend() const { return m_data + m_size; }

protected:
    T         m_data[Size];
    size_type m_size;
};

}   // namespace jam