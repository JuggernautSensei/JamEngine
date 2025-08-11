#pragma once

namespace jam
{

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