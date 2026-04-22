#include "pch.h"

#include "StringUtilities.h"

namespace jam
{

std::wstring ToLower(const std::wstring_view _wStr)
{
    std::wstring output;
    output.resize(_wStr.size() + 1);
    for (size_t i = 0; i < _wStr.size(); ++i)
    {
        output[i] = static_cast<wchar_t>(towlower(_wStr[i]));
    }
    return output;
}

std::string ToLower(const std::string_view _str)
{
    std::string output;
    output.resize(_str.size() + 1);
    for (size_t i = 0; i < _str.size(); ++i)
    {
        output[i] = static_cast<char>(tolower(_str[i]));
    }
    return output;
}

std::vector<std::string> Split(const std::string_view _str, const std::string_view _delimiter)
{
    std::vector<std::string> output;

    size_t start = 0;
    size_t end   = _str.find(_delimiter);
    while (end != std::string::npos)
    {
        output.emplace_back(_str.substr(start, end - start));
        start = end + _delimiter.length();
        end   = _str.find(_delimiter, start);
    }
    output.emplace_back(_str.substr(start, end));

    return output;
}

}   // namespace jam