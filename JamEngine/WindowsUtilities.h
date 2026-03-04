#pragma once

namespace Jam
{

enum class eWindowsErrorCodeType
{
    Errno,
    HRESULT,
    Shell,
};

[[nodiscard]] String  CreateWindowsErrorMessage(UInt32 _errorCode, eWindowsErrorCodeType _errorType);
[[nodiscard]] String  CreateWindowsLastErrorMessage();
[[nodiscard]] WString CreateWindowsErrorMessageW(UInt32 _errorCode, eWindowsErrorCodeType _errorType);
[[nodiscard]] WString CreateWindowsLastErrorMessageW();
[[nodiscard]] Error   ErrorFromPlatformErrorCode(UInt32 _err);

}   // namespace Jam