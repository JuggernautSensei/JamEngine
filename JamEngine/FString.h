#pragma once

namespace jam
{

template<size_t N>
class fixed_string
{
    static constexpr size_t k_bufferCapacity = N + 1;

public:
    using value_type = char;
    using size_type  = size_t;

    void set(const char* _str, const size_type _size)
    {
        m_len = std::min(N, _size);
        std::memcpy(m_data, _str, m_len);
        m_data[m_len] = '\0';
    }

    void append(const char* _str, const size_type _size)
    {
        size_type copySize = std::min(N - m_len, _size);
        std::memcpy(m_data + m_len, _str, copySize);
        m_len += copySize;
        m_data[m_len] = '\0';
    }

    void set_v(const char* _fmt, ...)
    {
        va_list args;
        va_start(args, _fmt);
        int written = std::vsnprintf(m_data, k_bufferCapacity, _fmt, args);
        va_end(args);
        m_len         = (written < 0) ? 0 : std::min(static_cast<size_type>(written), N);
        m_data[m_len] = '\0';
    }

    void append_v(const char* _fmt, ...)
    {
        va_list args;
        va_start(args, _fmt);
        int written = std::vsnprintf(m_data + m_len, k_bufferCapacity - m_len, _fmt, args);
        va_end(args);
        size_type appendLen = (written < 0) ? 0 : std::min(static_cast<size_type>(written), N - m_len);
        m_len += appendLen;
        m_data[m_len] = '\0';
    }

    void clear()
    {
        m_data[0] = '\0';
        m_len     = 0;
    }

    JAM_NODISCARD const char* c_str() const
    {
        return m_data;
    }

    JAM_NODISCARD size_type size() const
    {
        return m_len;
    }

    JAM_NODISCARD bool empty() const
    {
        return m_len == 0;
    }

    JAM_NODISCARD size_type capacity() const
    {
        return N;
    }

    JAM_NODISCARD char* data()
    {
        return m_data;
    }

    JAM_NODISCARD const char* data() const
    {
        return m_data;
    }

private:
    value_type m_data[k_bufferCapacity] {};
    size_type  m_len = 0;
};

}   // namespace jam