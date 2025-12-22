#include "pch.h"

#include "StringUtilities.h"

namespace jam
{

std::string ToLower(
    const StringView _str)
{
    std::string output;
    output.reserve(_str.GetLength());
    for (Int32 i = 0; i < _str.GetLength(); ++i)
    {
        const char c = ::tolower(_str[i]);
        output.push_back(c);
    }

    return output;
}

std::string ToUtf8(
    const WStringView _wstr)
{
    if (_wstr.IsEmpty())
    {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, _wstr.GetPtr(), _wstr.GetLength(), nullptr, 0, nullptr, nullptr);
    if (size == 0)
    {
        return {};
    }

    std::string result;
    result.resize(size);
    if (WideCharToMultiByte(CP_UTF8, 0, _wstr.GetPtr(), _wstr.GetLength(), result.data(), size, nullptr, nullptr) == 0)
    {
        return {};
    }

    return result;
}

std::wstring ToUtf16(
    const StringView _str)
{
    if (_str.IsEmpty())
    {
        return {};
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, _str.GetPtr(), _str.GetLength(), nullptr, 0);
    if (size == 0)
    {
        return {};
    }

    std::wstring result;
    result.resize(size);
    if (MultiByteToWideChar(CP_UTF8, 0, _str.GetPtr(), _str.GetLength(), result.data(), size) == 0)
    {
        return {};
    }

    return result;
}

}   // namespace jam