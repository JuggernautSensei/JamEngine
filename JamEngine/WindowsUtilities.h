#pragma once

namespace jam
{

NODISCARD std::string GetSystemErrorMessage(DWORD _errorCode);
NODISCARD std::string GetSystemLastErrorMessage();

}   // namespace jam