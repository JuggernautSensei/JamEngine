#include "pch.h"

#include "Debug.h"

#include "StackString.h"

namespace Jam
{

void PrintToDebugConsole(
    const StringView _msg)
{
    WString msg = {};
    msg.Encode(_msg);
    ::OutputDebugStringW(msg.GetPtr());
}

void PrintToMessageBox(
    const StringView _title,
    const StringView _msg)
{
    WString title = {};
    title.Encode(_title);

    WString msg = {};
    msg.Encode(_msg);

    ::MessageBoxW(nullptr, msg.GetPtr(), title.GetPtr(), MB_OK | MB_ICONINFORMATION);
}

}   // namespace Jam
