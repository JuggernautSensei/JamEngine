#include "pch.h"

#include "EntryPoint.h"

#include "Application.h"

namespace Jam
{

Int32 EntryPoint(const Int32 argc, char* argv[])
{
    Application::CreateInstance(argc, argv);
    const Int32 result = JAM_APP.Run();
    Application::DestroyInstance();
    return result;
}

}   // namespace Jam
