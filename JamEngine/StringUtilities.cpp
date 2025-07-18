#include "pch.h"

#include "StringUtilities.h"

namespace jam
{

std::wstring ConvertToWideString(const std::string_view _str)
{
    const char* str = _str.data();

    // 사이즈 계산
    const int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);

    std::wstring wStr;
    wStr.resize(size);

    // UTF8 -> UTF16
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wStr.data(), size);
    return wStr;
}

std::string ConvertToString(const std::wstring_view _wStr)
{
    const wchar_t* wStr = _wStr.data();

    // 사이즈 계산
    const int size = WideCharToMultiByte(CP_UTF8, 0, wStr, -1, nullptr, 0, nullptr, nullptr);

    std::string str;
    str.resize(size);

    // UTF16 -> UTF8
    WideCharToMultiByte(CP_UTF8, 0, wStr, -1, str.data(), size, nullptr, nullptr);
    return str;
}

std::wstring ToLower(std::wstring_view _wStr)
{
    std::wstring dst;
    std::ranges::transform(_wStr, std::back_inserter(dst), [](const wchar_t c)
                           { return tolower(c); });
    return dst;
}

std::string ToLower(std::string_view _str)
{
    std::string dst;
    std::ranges::transform(_str, std::back_inserter(dst), [](const char c)
                           { return tolower(c); });
    return dst;
}

}   // namespace jam