#include "pch.h"

#include "StringUtilities.h"

namespace jam
{

std::wstring ConvertToWideString(const std::string_view _str)
{
    StringToWideStringConverter conv;   // UTF-8 to UTF-16 converter
    conv.Convert(_str);
    return conv.Build();
}

std::string ConvertToString(const std::wstring_view _wStr)
{
    WideStringToStringConverter conv;   // UTF-16 to UTF-8 converter
    conv.Convert(_wStr);
    return conv.Build();
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