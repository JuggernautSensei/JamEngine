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
[[nodiscard]] String  MakeWindowsLastErrorMessage();
[[nodiscard]] WString MakeWindowsErrorMessageW(UInt32 _errorCode, eWindowsErrorCode _errorType);
[[nodiscard]] WString MakeWindowsLastErrorMessageW();
[[nodiscard]] Error   ErrorFromWindowsErrorCode(UInt32 _err);

}   // namespace Jam