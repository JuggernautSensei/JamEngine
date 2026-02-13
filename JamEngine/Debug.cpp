#include "pch.h"

#include "Debug.h"

namespace Jam
{

void PrintToDebugConsole(
    const StringView _msg)
{
    OutputDebugStringA(_msg.GetPtr());
}

}   // namespace Jam
