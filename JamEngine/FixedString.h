#pragma once

namespace jam
{

template<Int32 N>
class FixedString
{
    static_assert(N > 0, "FString capacity N must be > 0");
    static constexpr size_t k_cap = N + 1;

public:
    FixedString() = default;
    /* implicit */ FixedString(const char* _str) { Set(_str, '\0'); }
    FixedString(const char* _str, const Int32 _len) { Set(_str, _len); }

    /* implicit */ FixedString(const std::string& _str) { Set(_str.c_str(), _str.size()); }
    /* implicit */ FixedString(const std::string_view _sv) { Set(_sv.data(), _sv.size()); }

    ~FixedString() = default;

    template<size_t M>
    /* implicit */ FixedString(const FixedString<M>& _other)
    {
        Set(_other.c_str(), _other.size());
    }

    FixedString(const FixedString& _other)
        : m_len(_other.m_len)
    {
        std::memcpy(m_data, _other.m_data, m_len + 1);
    }

    FixedString(FixedString&& _other) noexcept
        : m_len(_other.m_len)
    {
        std::memcpy(m_data, _other.m_data, m_len + 1);
    }

    template<size_t M>
    /* implicit */ FixedString(FixedString<M>&& _other) noexcept
    {
        Set(_other.c_str(), _other.size());
    }

    FixedString& operator=(const FixedString& _other)
    {
        if (this != &_other)
        {
            m_len = _other.m_len;
            std::memcpy(m_data, _other.m_data, m_len + 1);
        }
        return *this;
    }

    FixedString& operator=(FixedString&& _other) noexcept
    {
        if (this != &_other)
        {
            m_len = _other.m_len;
            std::memcpy(m_data, _other.m_data, m_len + 1);
        }
        return *this;
    }

    template<size_t M>
    FixedString& operator=(const FixedString<M>& _other)
    {
        Set(_other.c_str(), _other.size());
        return *this;
    }

    template<size_t M>
    FixedString& operator=(FixedString<M>&& _other) noexcept
    {
        Set(_other.c_str(), _other.size());
        return *this;
    }

    FixedString& operator=(const char* _str)
    {
        Set(_str, '\0');
        return *this;
    }

    FixedString& operator=(const std::string& _str)
    {
        Set(_str.c_str(), _str.size());
        return *this;
    }

    FixedString& operator=(const std::string_view _sv)
    {
        Set(_sv.data(), _sv.size());
        return *this;
    }

    void Set(const char* _str, const Int32 _len)
    {
        assert(_str && "_str is nullptr");
        m_len = Min(N, _len);
        if (m_len)
        {
            std::memcpy(m_data, _str, m_len);
        }
        m_data[m_len] = '\0';
    }

    void Set(const char* _str, const char _end = '\0')
    {
        assert(_str && "_str is nullptr");
        Int32 len = 0;
        for (; len < N; ++len)
        {
            char c = _str[len];
            if (c == _end)
            {
                break;
            }
            m_data[len] = c;
        }
        m_data[len] = '\0';
        m_len       = len;
    }

    void Set(const std::string& _str)
    {
        Set(_str.c_str(), _str.size());
    }

    void Set(const std::string_view _sv)
    {
        Set(_sv.data(), _sv.size());
    }

    template<size_t M>
    void Set(const FixedString<M>& _other)
    {
        Set(_other.c_str(), _other.size());
    }

    void Append(const char* _str, const Int32 _len)
    {
        assert(_str && "_str is nullptr");
        const Int32 remain = N - m_len;
        const Int32 len    = Min(remain, _len);
        if (len)
        {
            std::memcpy(m_data + m_len, _str, len);
            m_len += len;
        }
        m_data[m_len] = '\0';
    }

    void Append(const char* _str, const char _end = '\0')
    {
        assert(_str && "_str is nullptr");
        const Int32 remain = N - m_len;
        Int32       cpyLen = 0;
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

    void Append(const std::string& _str)
    {
        Append(_str.c_str(), _str.size());
    }

    void Append(const std::string_view _sv)
    {
        Append(_sv.data(), _sv.size());
    }

    template<typename... Args>
    void Format(const std::format_string<Args...> _fmt, Args&&... _args)
    {
        const auto result = std::format_to_n(m_data, N, _fmt, std::forward<Args>(_args)...);
        m_len             = NarrowCast<Int32>(result.out - m_data);
        m_data[m_len]     = '\0';
    }

    void Printf(const char* _fmt, ...)
    {
        va_list args;
        va_start(args, _fmt);
        VPrintf(_fmt, args);
        va_end(args);
    }

    void VPrintf(const char* _fmt, va_list _args)
    {
        const Int32 written = vsnprintf_s(m_data, k_cap, _fmt, _args);
        m_len               = Clamp(written, 0, N);
        m_data[m_len]       = '\0';
    }

    void Clear()
    {
        m_data[0] = '\0';
        m_len     = 0;
    }

    bool operator==(const FixedString& _other) const
    {
        return m_len == _other.m_len && std::memcmp(m_data, _other.m_data, m_len) == 0;
    }

    template<size_t M>
    bool operator==(const FixedString<M>& _other) const
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

    bool operator!=(const FixedString& _other) const { return !(*this == _other); }

    template<size_t M>
    bool operator!=(const FixedString<M>& _other) const
    {
        return !(*this == _other);
    }

    bool operator!=(const char* _str) const { return !(*this == _str); }
    bool operator!=(const std::string& _str) const { return !(*this == _str); }
    bool operator!=(std::string_view _sv) const { return !(*this == _sv); }

    char& operator[](Int32 idx)
    {
        assert(idx < m_len && "Index out of bounds");
        return m_data[idx];
    }
    const char& operator[](Int32 idx) const
    {
        assert(idx < m_len && "Index out of bounds");
        return m_data[idx];
    }

    [[nodiscard]] const char* GetPtr() const { return m_data; }
    [[nodiscard]] StringView  ToStringView() const { return StringView { m_data, m_len }; }
    [[nodiscard]] std::string ToString() const { return std::string(m_data, m_len); }
    [[nodiscard]] Int32       GetLength() const { return m_len; }
    [[nodiscard]] bool        IsEmpty() const { return m_len == 0; }
    [[nodiscard]] Int32       Capacity() const { return N; }

    char& Front()
    {
        assert(m_len > 0 && "Empty string");
        return m_data[0];
    }
    const char& Front() const
    {
        assert(m_len > 0 && "Empty string");
        return m_data[0];
    }
    char& Back()
    {
        assert(m_len > 0 && "Empty string");
        return m_data[m_len - 1];
    }
    const char& Back() const
    {
        assert(m_len > 0 && "Empty string");
        return m_data[m_len - 1];
    }

    explicit operator std::string() const
    {
        return std::string(m_data, m_len);
    }

    explicit operator std::string_view() const
    {
        return std::string_view(m_data, m_len);
    }

private:
    char  m_data[k_cap] = {};
    Int32 m_len         = 0;
};

template<Int32 N>
bool operator==(const std::string& lhs, const FixedString<N>& rhs)
{
    return rhs == lhs;
}

template<Int32 N>
bool operator==(std::string_view lhs, const FixedString<N>& rhs)
{
    return rhs == lhs;
}

template<Int32 N>
bool operator!=(const std::string& lhs, const FixedString<N>& rhs)
{
    return rhs != lhs;
}

template<Int32 N>
bool operator!=(std::string_view lhs, const FixedString<N>& rhs)
{
    return rhs != lhs;
}

}   // namespace jam