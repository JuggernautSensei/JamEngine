#include "pch.h"

#include "StringUtilities.h"

#include "WindowsUtilities.h"

namespace jam
{

std::string ToLower(const StringView _str)
{
    const Int32 len = _str.GetLength();

    std::string result;
    result.reserve(len);
    for (Int32 i = 0; i < len; ++i)
    {
        const char c = std::tolower(_str[i]);
        result.push_back(c);
    }
    return result;
}

Optional<std::string> ToUtf8(
    const WStringView _wstr)
{
    if (_wstr.IsEmpty())
    {
        return {};
    }

    const int size = ::WideCharToMultiByte(CP_UTF8, 0, _wstr.GetPtr(), _wstr.GetLength(), nullptr, 0, nullptr, nullptr);
    if (size == 0)
    {
        GlobalBuffer::SetLastError("WideCharToMultiByte failed in ToUtf8. {}", CreateErrorMessageFromWin32LastError());
        return k_nullopt;
    }

    std::string result(size, '\0');
    if (::WideCharToMultiByte(CP_UTF8, 0, _wstr.GetPtr(), _wstr.GetLength(), result.data(), size, nullptr, nullptr) == 0)
    {
        GlobalBuffer::SetLastError("WideCharToMultiByte failed in ToUtf8. {}", CreateErrorMessageFromWin32LastError());
        return k_nullopt;
    }

    return result;
}

Optional<std::wstring> ToUtf16(
    const StringView _str)
{
    if (_str.IsEmpty())
    {
        return {};
    }

    const int size = ::MultiByteToWideChar(CP_UTF8, 0, _str.GetPtr(), _str.GetLength(), nullptr, 0);
    if (size == 0)
    {
        GlobalBuffer::SetLastError("MultiByteToWideChar failed in ToUtf16. {}", CreateErrorMessageFromWin32LastError());
        return k_nullopt;
    }

    std::wstring result(size, L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, _str.GetPtr(), _str.GetLength(), result.data(), size) == 0)
    {
        GlobalBuffer::SetLastError("MultiByteToWideChar failed in ToUtf16. {}", CreateErrorMessageFromWin32LastError());
        return k_nullopt;
    }

    return result;
}

}   // namespace jam