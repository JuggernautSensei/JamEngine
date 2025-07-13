#pragma once

namespace jam
{

NODISCARD std::string GetSystemErrorMessage(const DWORD _errorCode);
NODISCARD std::string GetSystemLastErrorMessage();

}   // namespace jam