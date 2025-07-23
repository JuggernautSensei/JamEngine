#include "pch.h"

#include "WindowsUtilities.h"

namespace jam
{

std::string GetSystemErrorMessage(const DWORD _errorCode)
{
    constexpr DWORD k_dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

    LPVOID      lpMsgBuf = nullptr;
    const DWORD dwSize   = FormatMessageA(k_dwFlags, nullptr, _errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, nullptr);
    if (dwSize == 0)
    {
        return "Unknown error";
    }
    std::string message(static_cast<char*>(lpMsgBuf), dwSize);
    LocalFree(lpMsgBuf);
    return message;
}

std::string GetSystemLastErrorMessage()
{
    return GetSystemErrorMessage(GetLastError());
}

}   // namespace jam