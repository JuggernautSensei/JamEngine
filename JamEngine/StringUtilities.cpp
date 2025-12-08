#include "pch.h"

#include "StringUtilities.h"

#include "FString.h"

namespace jam
{

FString<1024> g_tmpBuffer;

std::wstring ToLower(const std::wstring_view _wStr)
{
    std::wstring output;
    output.reserve(_wStr.size());
    for (size_t i = 0; i < _wStr.size(); ++i)
    {
        const wchar_t c = ::towlower(_wStr[i]);
        output.push_back(c);
    }

    return output;
}

std::string ToLower(const std::string_view _str)
{
    std::string output;
    output.reserve(_str.size());
    for (size_t i = 0; i < _str.size(); ++i)
    {
        const char c = ::tolower(_str[i]);
        output.push_back(c);
    }

    return output;
}

std::string ToUTF8(const std::wstring_view _wstr)
{
    if (_wstr.empty())
    {
        return {};
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, _wstr.data(), static_cast<int>(_wstr.size()), nullptr, 0, nullptr, nullptr);
    if (size == 0)
    {
        return {};
    }

    std::string result;
    result.resize(size);
    if (WideCharToMultiByte(CP_UTF8, 0, _wstr.data(), static_cast<int>(_wstr.size()), result.data(), size, nullptr, nullptr) == 0)
    {
        return {};
    }

    return result;
}

std::wstring ToUTF16(const std::string_view _str)
{
    if (_str.empty())
    {
        return {};
    }

    int size = MultiByteToWideChar(CP_UTF8, 0, _str.data(), static_cast<int>(_str.size()), nullptr, 0);
    if (size == 0)
    {
        return {};
    }

    std::wstring result;
    result.resize(size);
    if (MultiByteToWideChar(CP_UTF8, 0, _str.data(), static_cast<int>(_str.size()), result.data(), size) == 0)
    {
        return {};
    }

    return result;
}

const char* FormatStringToTmpBuffer(const char* _fmt, ...)
{
    va_list ap;
    va_start(ap, _fmt);
    g_tmpBuffer.format_v(_fmt, ap);
    va_end(ap);
    return g_tmpBuffer.c_str();
}

const char* FormatStringToTmpBufferV(const char* _fmt, const va_list _args)
{
    g_tmpBuffer.format_v(_fmt, _args);
    return g_tmpBuffer.c_str();
}

}   // namespace jam