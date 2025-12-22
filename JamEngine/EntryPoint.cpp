#include "pch.h"

#include "EntryPoint.h"

#include "Application.h"

namespace jam
{

Int32 EntryPoint(const Int32 argc, char* argv[])
{
    Application::Create(argc, argv);
    const Int32 result = Application::GetInstance().Run();
    Application::Destroy();
    return result;
}

}   // namespace jam
