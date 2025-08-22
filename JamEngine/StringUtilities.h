#pragma once

namespace jam
{

template<size_t N>
struct StringLiteral
{
    explicit constexpr StringLiteral(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }

    char value[N];
};

class WideStringToStringConverter
{
public:
    void Convert(const std::wstring_view _wstr)
    {
        m_buf.clear();   // 내부 버퍼 초기화
        const wchar_t* wStr = _wstr.data();
        const int      size = WideCharToMultiByte(CP_UTF8, 0, wStr, -1, nullptr, 0, nullptr, nullptr);   // 사이즈 계산
        m_buf.resize(size);                                                                              // 버퍼 크기 조정
        WideCharToMultiByte(CP_UTF8, 0, wStr, -1, m_buf.data(), size, nullptr, nullptr);                 // 변환 수행
    }

    NODISCARD std::string Build() const { return m_buf; }
    NODISCARD std::string_view GetBufferView() const { return m_buf; }

private:
    std::string m_buf;
};

class StringToWideStringConverter
{
public:
    void Convert(const std::string_view _str)
    {
        m_buf.clear();   // 내부 버퍼 초기화
        const char* str  = _str.data();
        const int   size = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);   // 사이즈 계산
        m_buf.resize(size);                                                        // 버퍼 크기 조정
        MultiByteToWideChar(CP_UTF8, 0, str, -1, m_buf.data(), size);              // 변환 수행
    }

    NODISCARD std::wstring Build() const { return m_buf; }
    NODISCARD std::wstring_view GetBufferView() const { return m_buf; }

private:
    std::wstring m_buf;   // 내부 버퍼
};

NODISCARD std::wstring ConvertToWideString(std::string_view _str);
NODISCARD std::string ConvertToString(std::wstring_view _wStr);

NODISCARD std::wstring ToLower(std::wstring_view _wStr);
NODISCARD std::string ToLower(std::string_view _str);

NODISCARD constexpr UInt32 HashOf(const std::string_view _str)
{
    return entt::hashed_string { _str.data(), _str.size() }.value();
}

NODISCARD constexpr UInt32 HashOf(const std::wstring_view _wStr)
{
    return entt::hashed_wstring { _wStr.data(), _wStr.size() }.value();
}

NODISCARD constexpr UInt32 operator""_hs(const char* _str, const size_t _size)
{
    return HashOf(std::string_view { _str, _size });
}

NODISCARD constexpr UInt32 operator""_hs(const wchar_t* _str, const size_t _size)
{
    return HashOf(std::wstring_view { _str, _size });
}

}   // namespace jam