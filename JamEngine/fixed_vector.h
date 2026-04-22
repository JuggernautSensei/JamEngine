#pragma once

namespace jam
{

template<typename T, size_t Size_>
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

    template<typename... Args>
    void emplace_back(Args&&... args)
    {
        if (!full())
        {
            m_data[m_size] = T(std::forward<Args>(args)...);
            m_size++;
        }
        else
        {
            JAM_ASSERT(0, "fixed_vector is full. Cannot emplace_back.");
        }
    }

    void push_back(const T& value)
    {
        if (!full())
        {
            m_data[m_size] = value;
            m_size++;
        }
        else
        {
            JAM_ASSERT(0, "fixed_vector is full. Cannot push_back.");
        }
    }

    void push_back(T&& value)
    {
        if (!full())
        {
            m_data[m_size] = std::move(value);
            m_size++;
        }
        else
        {
            JAM_ASSERT(0, "fixed_vector is full. Cannot push_back.");
        }
    }

    void pop_back()
    {
        if (!empty())
        {
            m_size--;
        }
        else
        {
            JAM_ASSERT(0, "fixed_vector is empty. Cannot pop_back.");
        }
    }

    void clear() noexcept
    {
        // size를 0으로 초기화하여 모든 요소가 비활성화된 것처럼 처리합니다.
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
    JAM_NODISCARD constexpr size_type capacity() const { return Size_; }
    JAM_NODISCARD bool                full() const { return m_size == Size_; }

    JAM_NODISCARD iterator       begin() { return m_data; }
    JAM_NODISCARD iterator       end() { return m_data + m_size; }
    JAM_NODISCARD const_iterator begin() const { return m_data; }
    JAM_NODISCARD const_iterator end() const { return m_data + m_size; }
    JAM_NODISCARD const_iterator cbegin() const { return m_data; }
    JAM_NODISCARD const_iterator cend() const { return m_data + m_size; }

protected:
    T         m_data[Size_];
    size_type m_size;
};

}   // namespace jam