#pragma once

namespace jam
{

template<size_t N>
class FString
{
    static_assert(N > 0, "FString capacity N must be > 0");
    static constexpr size_t k_cap = N + 1;

public:
    using value_type = char;
    using size_type  = size_t;

    FString() = default;
    /* implicit */ FString(const char* _str) { set(_str, '\0'); }
    FString(const char* _str, const size_type _len) { set(_str, _len); }

    /* implicit */ FString(const std::string& _str) { set(_str.c_str(), _str.size()); }
    /* implicit */ FString(const std::string_view _sv) { set(_sv.data(), _sv.size()); }

    ~FString() = default;

    template<size_t M>
    /* implicit */ FString(const FString<M>& _other)
    {
        set(_other.c_str(), _other.size());
    }

    FString(const FString& _other)
        : m_len(_other.m_len)
    {
        std::memcpy(m_data, _other.m_data, m_len + 1);
    }

    FString(FString&& _other) noexcept
        : m_len(_other.m_len)
    {
        std::memcpy(m_data, _other.m_data, m_len + 1);
    }

    template<size_t M>
    /* implicit */ FString(FString<M>&& _other) noexcept
    {
        set(_other.c_str(), _other.size());
    }

    FString& operator=(const FString& _other)
    {
        if (this != &_other)
        {
            m_len = _other.m_len;
            std::memcpy(m_data, _other.m_data, m_len + 1);
        }
        return *this;
    }

    FString& operator=(FString&& _other) noexcept
    {
        if (this != &_other)
        {
            m_len = _other.m_len;
            std::memcpy(m_data, _other.m_data, m_len + 1);
        }
        return *this;
    }

    template<size_t M>
    FString& operator=(const FString<M>& _other)
    {
        set(_other.c_str(), _other.size());
        return *this;
    }

    template<size_t M>
    FString& operator=(FString<M>&& _other) noexcept
    {
        set(_other.c_str(), _other.size());
        return *this;
    }

    FString& operator=(const char* _str)
    {
        set(_str, '\0');
        return *this;
    }

    FString& operator=(const std::string& _str)
    {
        set(_str.c_str(), _str.size());
        return *this;
    }

    FString& operator=(const std::string_view _sv)
    {
        set(_sv.data(), _sv.size());
        return *this;
    }

    void set(const char* _str, const size_type _len)
    {
        assert(_str && "_str is nullptr");
        m_len = Min(N, _len);
        if (m_len) std::memcpy(m_data, _str, m_len);
        m_data[m_len] = '\0';
    }

    void set(const char* _str, const char _end = '\0')
    {
        assert(_str && "_str is nullptr");
        size_type len = 0;
        for (; len < N; ++len)
        {
            char c = _str[len];
            if (c == _end)
                break;
            m_data[len] = c;
        }
        m_data[len] = '\0';
        m_len       = len;
    }

    void set(const std::string& _str)
    {
        set(_str.c_str(), _str.size());
    }

    void set(const std::string_view _sv)
    {
        set(_sv.data(), _sv.size());
    }

    template<size_t M>
    void set(const FString<M>& _other)
    {
        set(_other.c_str(), _other.size());
    }

    void format(const char* _fmt, ...)
    {
        assert(_fmt && "fmt is nullptr");
        va_list ap;
        va_start(ap, _fmt);
        format_v(_fmt, ap);
        va_end(ap);
    }

    void format_v(const char* _fmt, const va_list _args)
    {
        assert(_fmt && "fmt is nullptr");
        int len = std::vsnprintf(m_data, k_cap, _fmt, _args);
        if (len < 0)
        {
            m_data[0] = '\0';
            m_len     = 0;
            return;
        }
        m_len         = Min(static_cast<size_type>(len), N);
        m_data[m_len] = '\0';
    }

    void append(const char* _str, const size_type _len)
    {
        assert(_str && "_str is nullptr");
        const size_type remain = N - m_len;
        const size_type len    = Min(remain, _len);
        if (len)
        {
            std::memcpy(m_data + m_len, _str, len);
            m_len += len;
        }
        m_data[m_len] = '\0';
    }

    void append(const char* _str, const char _end = '\0')
    {
        assert(_str && "_str is nullptr");
        const size_type remain = N - m_len;
        size_type       cpyLen = 0;
        for (; cpyLen < remain; ++cpyLen)
        {
            char c = _str[cpyLen];
            if (c == _end)
                break;
            m_data[m_len + cpyLen] = c;
        }
        m_len += cpyLen;
        m_data[m_len] = '\0';
    }

    void append(const std::string& _str)
    {
        append(_str.c_str(), _str.size());
    }

    void append(const std::string_view _sv)
    {
        append(_sv.data(), _sv.size());
    }

    void format_appand(const char* fmt, ...)
    {
        if (!fmt) return;
        va_list ap;
        va_start(ap, fmt);
        format_appand_v(fmt, ap);
        va_end(ap);
    }

    void format_appand_v(const char* _fmt, const va_list _args)
    {
        assert(_fmt && "fmt is nullptr");
        const size_type remain = k_cap - m_len;
        if (remain <= 1) return;
        int len = std::vsnprintf(m_data + m_len, remain, _fmt, _args);
        if (len > 0)
        {
            const size_type written = static_cast<size_type>(len);
            m_len += Min(written, remain - 1);
        }
        m_data[m_len] = '\0';
    }

    void clear()
    {
        m_data[0] = '\0';
        m_len     = 0;
    }

    bool operator==(const FString& _other) const
    {
        return m_len == _other.m_len && std::memcmp(m_data, _other.m_data, m_len) == 0;
    }

    template<size_t M>
    bool operator==(const FString<M>& _other) const
    {
        return m_len == _other.size() && std::memcmp(m_data, _other.c_str(), m_len) == 0;
    }

    bool operator==(const char* _str) const
    {
        return std::strcmp(m_data, _str) == 0;
    }

    bool operator==(const std::string& _str) const
    {
        return m_len == _str.size() && std::memcmp(m_data, _str.c_str(), m_len) == 0;
    }

    bool operator==(const std::string_view _sv) const
    {
        return m_len == _sv.size() && std::memcmp(m_data, _sv.data(), m_len) == 0;
    }

    bool operator!=(const FString& _other) const { return !(*this == _other); }

    template<size_t M>
    bool operator!=(const FString<M>& _other) const
    {
        return !(*this == _other);
    }

    bool operator!=(const char* _str) const { return !(*this == _str); }
    bool operator!=(const std::string& _str) const { return !(*this == _str); }
    bool operator!=(std::string_view _sv) const { return !(*this == _sv); }

    char& operator[](size_type idx)
    {
        assert(idx < m_len && "Index out of bounds");
        return m_data[idx];
    }
    const char& operator[](size_type idx) const
    {
        assert(idx < m_len && "Index out of bounds");
        return m_data[idx];
    }

    char*       begin() { return m_data; }
    char*       end() { return m_data + m_len; }
    const char* begin() const { return m_data; }
    const char* end() const { return m_data + m_len; }

    const char* c_str() const { return m_data; }
    size_type   size() const { return m_len; }
    bool        empty() const { return m_len == 0; }
    size_type   capacity() const { return N; }
    char*       data() { return m_data; }
    const char* data() const { return m_data; }

    char& front()
    {
        assert(m_len > 0 && "Empty string");
        return m_data[0];
    }
    const char& front() const
    {
        assert(m_len > 0 && "Empty string");
        return m_data[0];
    }
    char& back()
    {
        assert(m_len > 0 && "Empty string");
        return m_data[m_len - 1];
    }
    const char& back() const
    {
        assert(m_len > 0 && "Empty string");
        return m_data[m_len - 1];
    }

    operator std::string() const
    {
        return std::string(m_data, m_len);
    }

    operator std::string_view() const
    {
        return std::string_view(m_data, m_len);
    }

private:
    value_type m_data[k_cap] {};
    size_type  m_len = 0;
};

template<size_t N>
bool operator==(const std::string& lhs, const FString<N>& rhs)
{
    return rhs == lhs;
}

template<size_t N>
bool operator==(std::string_view lhs, const FString<N>& rhs)
{
    return rhs == lhs;
}

template<size_t N>
bool operator!=(const std::string& lhs, const FString<N>& rhs)
{
    return rhs != lhs;
}

template<size_t N>
bool operator!=(std::string_view lhs, const FString<N>& rhs)
{
    return rhs != lhs;
}

}   // namespace jam