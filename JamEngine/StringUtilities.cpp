#include "pch.h"

#include "StringUtilities.h"

namespace jam
{

std::wstring ToLower(const std::wstring_view _wStr)
{
    std::wstring output;
    output.resize(_wStr.size());
    for (size_t i = 0; i < _wStr.size(); ++i)
    {
        output[i] = static_cast<wchar_t>(towlower(_wStr[i]));
    }
    return output;
}

std::string ToLower(const std::string_view _str)
{
    std::string output;
    output.resize(_str.size());
    for (size_t i = 0; i < _str.size(); ++i)
    {
        output[i] = static_cast<char>(tolower(_str[i]));
    }
    return output;
}

Result<std::string> ToUTF8(const std::wstring_view _wstr)
{
    if (_wstr.empty())
    {
        return {};
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, _wstr.data(), static_cast<int>(_wstr.size()), nullptr, 0, nullptr, nullptr);
    if (size == 0)
    {
        return eErrorCode::RuntimeError;
    }

    std::string result;
    result.resize(size);
    if (WideCharToMultiByte(CP_UTF8, 0, _wstr.data(), static_cast<int>(_wstr.size()), result.data(), size, nullptr, nullptr) == 0)
    {
        return eErrorCode::RuntimeError;
    }

    return result;
}

Result<std::wstring> ToUTF16(const std::string_view _str)
{
    if (_str.empty())
    {
        return {};
    }

    int size = MultiByteToWideChar(CP_UTF8, 0, _str.data(), static_cast<int>(_str.size()), nullptr, 0);
    if (size == 0)
    {
        return eErrorCode::RuntimeError;
    }

    std::wstring result;
    result.resize(size);
    if (MultiByteToWideChar(CP_UTF8, 0, _str.data(), static_cast<int>(_str.size()), result.data(), size) == 0)
    {
        return eErrorCode::RuntimeError;
    }

    return result;
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