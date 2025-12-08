#pragma once

namespace jam
{

[[nodiscard]] constexpr UInt32 HashOf(const std::string_view _str)
{
    return entt::hashed_string { _str.data(), _str.size() }.value();
}

[[nodiscard]] constexpr UInt32 HashOf(const std::wstring_view _wStr)
{
    return entt::hashed_wstring { _wStr.data(), _wStr.size() }.value();
}

[[nodiscard]] constexpr UInt32 operator""_hs(const char* _str, const size_t _size)
{
    return HashOf(std::string_view { _str, _size });
}

[[nodiscard]] constexpr UInt32 operator""_hs(const wchar_t* _str, const size_t _size)
{
    return HashOf(std::wstring_view { _str, _size });
}

[[nodiscard]] std::wstring ToLower(std::wstring_view _wStr);
[[nodiscard]] std::string  ToLower(std::string_view _str);
[[nodiscard]] std::string  ToUTF8(std::wstring_view _wstr);
[[nodiscard]] std::wstring ToUTF16(std::string_view _str);

[[nodiscard]] const char* FormatStringToTmpBuffer(const char* _fmt, ...);
[[nodiscard]] const char* FormatStringToTmpBufferV(const char* _fmt, va_list _args);

}   // namespace jam