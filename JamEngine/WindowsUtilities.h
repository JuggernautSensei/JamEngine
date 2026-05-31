#pragma once

namespace Jam
{

enum class eWindowsErrorCode
{
    Errno,
    HRESULT,
    Shell,
};

[[nodiscard]] String  MakeWindowsErrorMessage(UInt32 _errorCode, eWindowsErrorCode _errorType);
[[nodiscard]] String  CreateWindowsLastErrorMessage();
[[nodiscard]] WString CreateWindowsErrorMessageW(UInt32 _errorCode, eWindowsErrorCode _errorType);
[[nodiscard]] WString CreateWindowsLastErrorMessageW();
[[nodiscard]] Error   ErrorFromPlatformErrorCode(UInt32 _err);

}   // namespace Jam