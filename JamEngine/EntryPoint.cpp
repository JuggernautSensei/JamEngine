#include "pch.h"

#include "EntryPoint.h"

#include "Application.h"

namespace jam::detail
{

int EntryPoint(const int argc, char* argv[])
{
    using namespace jam;
    const CommandLineArguments args = CommandLineArguments { argc, argv };
    Application::Create(args);
    const int result = GetApplication().Run();
    Application::Destroy();
    return result;
}

}   // namespace jam::detail
