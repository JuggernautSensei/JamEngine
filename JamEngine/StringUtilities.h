#pragma once

namespace jam
{

JAM_NODISCARD constexpr UInt32 HashOf(const std::string_view _str)
{
    return entt::hashed_string { _str.data(), _str.size() }.value();
}

JAM_NODISCARD constexpr UInt32 HashOf(const std::wstring_view _wStr)
{
    return entt::hashed_wstring { _wStr.data(), _wStr.size() }.value();
}

JAM_NODISCARD constexpr UInt32 operator""_hs(const char* _str, const size_t _size)
{
    return HashOf(std::string_view { _str, _size });
}

JAM_NODISCARD constexpr UInt32 operator""_hs(const wchar_t* _str, const size_t _size)
{
    return HashOf(std::wstring_view { _str, _size });
}

JAM_NODISCARD std::wstring ToLower(std::wstring_view _wStr);
JAM_NODISCARD std::string ToLower(std::string_view _str);
JAM_NODISCARD Result<std::string> ToUTF8(std::wstring_view _wstr);
JAM_NODISCARD Result<std::wstring> ToUTF16(std::string_view _str);
JAM_NODISCARD std::vector<std::string> Split(std::string_view _str, std::string_view _delimiter);

}   // namespace jam