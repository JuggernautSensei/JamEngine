#pragma once

namespace jam
{

template<size_t N>
class FString
{
public:
    constexpr FString() = default;

    explicit constexpr FString(const std::string_view _str)
    {
        Set(_str);
    }

    explicit constexpr FString(const char* _str)
    {
        Set(_str);
    }

    explicit constexpr operator std::string_view() const
    {
        return { m_str, m_size };
    }

    constexpr bool operator==(const FString& _other) const
    {
        return std::string_view(*this) == std::string_view(_other);
    }

    constexpr bool operator!=(const FString& _other) const
    {
        return !(*this == _other);
    }

    constexpr void Set(const std::string_view _str)
    {
        Set(_str.data(), _str.size());
    }

    void Set(const char* _str)
    {
        const size_t len = std::strlen(_str);
        Set(_str, len);
    }

    constexpr void Set(const char* _str, const size_t _size)
    {
        m_size = std::min(_size, k_availableCapacity);
        std::memcpy(m_str, _str, m_size);
        m_str[m_size] = '\0';
    }

    void Append(const FString& other)
    {
        const size_t len = other.Size();
        m_size     = std::min(m_size + len, k_availableCapacity);
        std::memcpy(m_str + m_size, other.CStr(), len);
        m_str[m_size] = '\0';
    }

    constexpr void Clear()
    {
        m_size   = 0;
        m_str[0] = '\0';
    }

    NODISCARD constexpr const char* CStr() const { return m_str; }
    NODISCARD constexpr const char* Data() const { return m_str; }
    NODISCARD constexpr size_t      Size() const { return m_size; }
    NODISCARD constexpr bool        Empty() const { return m_size == 0; }
    NODISCARD constexpr size_t      Capacity() const { return k_availableCapacity; }

private:
    static constexpr size_t k_capacity          = N + 1;
    static constexpr size_t k_availableCapacity = N;

    char   m_str[k_capacity] = {};
    size_t m_size            = 0;
};

NODISCARD std::wstring ConvertToWideString(std::string_view _str);
NODISCARD std::string ConvertToString(std::wstring_view _wStr);

NODISCARD constexpr UInt32 HashString(const std::string_view _str)
{
    return entt::hashed_string { _str.data(), _str.size() }.value();
}

NODISCARD constexpr UInt32 HashWideString(const std::wstring_view _wStr)
{
    return entt::hashed_wstring { _wStr.data(), _wStr.size() }.value();
}

NODISCARD constexpr UInt32 operator""_hs(const char* _str, const size_t _size)
{
    return HashString(std::string_view { _str, _size });
}

NODISCARD constexpr UInt32 operator""_hs(const wchar_t* _str, const size_t _size)
{
    return HashWideString(std::wstring_view { _str, _size });
}

}   // namespace jam